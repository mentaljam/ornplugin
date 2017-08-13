#include "ornzypp.h"
#include "orn.h"

#include <solv/repo_solv.h>

#include <QtConcurrent/QtConcurrent>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingCallWatcher>

#include <QDebug>

#define SSU_METHOD_DISPLAYNAME QStringLiteral("displayName")
#define SSU_CONFIG_PATH        QStringLiteral("/etc/ssu/ssu.ini")
#define SSU_REPOS_GROUP        QStringLiteral("repository-urls")
#define SSU_DISABLED_KEY       QStringLiteral("disabled-repos")

const QString OrnZypp::ssuInterface(QStringLiteral("org.nemo.ssu"));
const QString OrnZypp::ssuPath(QStringLiteral("/org/nemo/ssu"));
const QString OrnZypp::ssuModifyRepo(QStringLiteral("modifyRepo"));
const QString OrnZypp::ssuAddRepo(QStringLiteral("addRepo"));
const QString OrnZypp::ssuUpdateRepos(QStringLiteral("updateRepos"));
const QString OrnZypp::repoBaseUrl(QStringLiteral("https://sailfish.openrepos.net/%0/personal/main"));
const QString OrnZypp::repoNamePrefix(QStringLiteral("openrepos-"));
const QString OrnZypp::installed(QStringLiteral("installed"));
const QString OrnZypp::solvTmpl(QStringLiteral("/var/cache/zypp/solv/%0/solv"));
const int OrnZypp::repoNamePrefixLength = OrnZypp::repoNamePrefix.length();
OrnZypp *OrnZypp::gInstance = 0;

OrnZypp::OrnZypp(QObject *parent) :
    QObject(parent),
    mBusy(false),
    mAvailableFetcher(0),
    mInstalledFetcher(0),
    mUpdatesFetcher(0)
{
    // Rescan packages on repos data fetched
    connect(this, &OrnZypp::endRepoFetching, this, &OrnZypp::getAllPackages);
    // Refetch repos data if repos have been changed
    connect(this, &OrnZypp::repoModified, this, &OrnZypp::onRepoModified);
}

OrnZypp *OrnZypp::instance()
{
    if (!gInstance)
    {
        gInstance = new OrnZypp(qApp);
    }
    return gInstance;
}

OrnZypp::RepoStatus OrnZypp::repoStatus(const QString &alias) const
{
    if (!mRepos.contains(alias))
    {
        return RepoNotInstalled;
    }
    return mRepos[alias].enabled ? RepoEnabled : RepoDisabled;
}

OrnZypp::RepoList OrnZypp::repoList() const
{
    RepoList repos;
    for (auto it = mRepos.constBegin(); it != mRepos.constEnd(); ++it)
    {
        auto alias = it.key();
        repos << Repo{ it.value().enabled, alias, alias.mid(repoNamePrefixLength) };
    }
    return repos;
}

bool OrnZypp::isAvailable(const QString &packageName) const
{
    return mAvailablePackages.contains(packageName);
}

QStringList OrnZypp::availablePackages(const QString &packageName) const
{
    auto available = mAvailablePackages.values(packageName);
    // Append also an installed package
    auto installed = this->installedPackage(packageName);
    if (!installed.isEmpty())
    {
        available.append(installed);
    }
    return available;
}

bool OrnZypp::isInstalled(const QString &packageName) const
{
    return mInstalledPackages.contains(packageName);
}

QString OrnZypp::installedPackage(const QString &packageName) const
{
    return mInstalledPackages.value(packageName);
}

bool OrnZypp::updatesAvailable() const
{
    return !mUpdates.isEmpty();
}

bool OrnZypp::hasUpdate(const QString &packageName) const
{
    return mUpdates.contains(packageName);
}

QString OrnZypp::updatePackage(const QString &packageName) const
{
    return mUpdates.value(packageName);
}

QString OrnZypp::packageRepo(const QString &name) const
{
    for (auto r = mRepos.constBegin(); r != mRepos.constEnd(); ++r)
    {
        if (r.value().packages.contains(name))
        {
            return r.key();
        }
    }
    return QString();
}

QString OrnZypp::deviceModel()
{
    auto methodCall = QDBusMessage::createMethodCall(
                ssuInterface,
                ssuPath,
                ssuInterface,
                SSU_METHOD_DISPLAYNAME);
    // Ssu::DeviceModel = 1
    methodCall.setArguments({ 1 });
    qDebug() << "Calling" << methodCall;
    auto call = QDBusConnection::systemBus().call(methodCall, QDBus::BlockWithGui, 7000);
    return call.arguments().first().toString();
}

bool OrnZypp::getInstalledApps()
{
    if (mBusy)
    {
        qWarning() << "OrnZypp is already busy";
        return false;
    }
    mBusy = true;
    QtConcurrent::run(this, &OrnZypp::pInstalledApps);
    return true;
}

void OrnZypp::fetchRepos()
{
    QtConcurrent::run(this, &OrnZypp::pFetchRepos);
}

void OrnZypp::addRepo(const QString &author)
{
    auto alias = repoNamePrefix + author;
    auto watcher = pDbusCall(ssuAddRepo, QVariantList{ alias, repoBaseUrl.arg(author) });
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, alias]()
    {
        qDebug() << "Repo" << alias << "have been added";
        mRepos.insert(alias, RepoMeta{ true, QSet<QString>() });
        emit this->repoModified(alias, AddRepo);
    });
}

void OrnZypp::modifyRepo(const QString &alias, const RepoAction &action)
{
    auto watcher = pDbusCall(ssuModifyRepo, QVariantList{ action, alias });
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, alias, action]()
    {
        qDebug() << "Repo" << alias << "have been modified with" << action;
        switch (action)
        {
        case RemoveRepo:
            mRepos.remove(alias);
            break;
        case DisableRepo:
            mRepos[alias].enabled = false;
            break;
        case EnableRepo:
            mRepos[alias].enabled = true;
            break;
        default:
            break;
        }
        emit this->repoModified(alias, action);
    });
}

void OrnZypp::enableRepos(bool enable)
{
    QStringList repos;
    QSettings ssuSettings(SSU_CONFIG_PATH, QSettings::IniFormat);

    auto disabled = ssuSettings.value(SSU_DISABLED_KEY).toStringList();
    if (enable)
    {
        repos = disabled;
    }
    else
    {
        ssuSettings.beginGroup(SSU_REPOS_GROUP);
        auto enabled = ssuSettings.childKeys();
        for (const auto &repo : disabled)
        {
            enabled.removeOne(repo);
        }
        repos = enabled;
    }

    auto action = enable ? EnableRepo : DisableRepo;
    for (const auto &repo : repos)
    {
        this->modifyRepo(repo, action);
    }
}

void OrnZypp::refreshRepos(bool force)
{
    auto t = Orn::transaction();
    qDebug() << "Refreshing repos with" << t << "method refreshCache()";
    t->refreshCache(force);
}

void OrnZypp::refreshRepo(const QString &alias, bool force)
{
    auto t = Orn::transaction();
    qDebug() << "Refreshing repo" << alias << "with" << t << "method repoSetData()";
    t->repoSetData(alias, QStringLiteral("refresh-now"),
                   force ? QStringLiteral("true") : QStringLiteral("false"));
}

void OrnZypp::getAvailablePackages()
{
    this->pPrepareFetching(mAvailableFetcher);
    qDebug() << "Getting all available packages with" << mAvailableFetcher
             << "method getPackages(FilterSupported)";
    connect(mAvailableFetcher, &PackageKit::Transaction::finished, [this]
    {
        this->mAvailableFetcher = 0;
        emit this->availablePackagesChanged();
    });
    mAvailablePackages.clear();
    mAvailableFetcher->getPackages(PackageKit::Transaction::FilterSupported);
}

void OrnZypp::getInstalledPackages()
{
    this->pPrepareFetching(mInstalledFetcher);
    qDebug() << "Getting all installed packages with" << mInstalledFetcher
             << "method getPackages(FilterInstalled)";
    connect(mInstalledFetcher, &PackageKit::Transaction::finished, [this]
    {
        this->mInstalledFetcher = 0;
        emit this->installedPackagesChanged();
    });
    mInstalledPackages.clear();
    mInstalledFetcher->getPackages(PackageKit::Transaction::FilterInstalled);
}

void OrnZypp::getUpdates()
{
    this->pPrepareFetching(mUpdatesFetcher);
    qDebug() << "Getting all updates with" << mUpdatesFetcher
             << "method getUpdates()";
    connect(mUpdatesFetcher, &PackageKit::Transaction::finished, [this]
    {
        this->mUpdatesFetcher = 0;
        emit this->updatesChanged();
    });
    mUpdates.clear();
    mUpdatesFetcher->getUpdates();
}

void OrnZypp::getAllPackages()
{
    this->getAvailablePackages();
    this->getInstalledPackages();
    this->getUpdates();
}

void OrnZypp::installPackage(const QString &packageId)
{
    auto t = Orn::transaction();
    connect(t, &PackageKit::Transaction::finished, [this, packageId]()
    {
        auto name = PackageKit::Transaction::packageName(packageId);
        mInstalledPackages.insert(name, packageId);
        emit this->installedPackagesChanged();
        mUpdates.remove(name);
        emit this->updatesChanged();
        emit this->packageInstalled(packageId);
    });
    qDebug() << "Installing package" << packageId << "with" << t << "method installPackage()";
    t->installPackage(packageId);
}

void OrnZypp::removePackage(const QString &packageId)
{
    auto t = Orn::transaction();
    connect(t, &PackageKit::Transaction::finished, [this, packageId]()
    {
        auto name = PackageKit::Transaction::packageName(packageId);
        if (mInstalledPackages.remove(name))
        {
            emit this->installedPackagesChanged();
        }
        emit this->packageRemoved(packageId);
    });
    qDebug() << "Removing package" << packageId << "with" << t << "method installPackage()";
    t->removePackage(packageId);
}

void OrnZypp::updateAll()
{
    PackageKit::Transaction *t = 0;
    this->pPrepareFetching(t);
    connect(t, &PackageKit::Transaction::finished, this, &OrnZypp::getAllPackages);
    t->updatePackages(mUpdates.values());
}

void OrnZypp::onRepoModified(const QString &alias, const RepoAction &action)
{
    switch (action)
    {
    case AddRepo:
    case EnableRepo:
    {
        auto t = Orn::transaction();
        connect(t, &PackageKit::Transaction::finished, [this, alias]()
        {
            this->pFetchRepoPackages(alias);
            this->getAllPackages();
        });
        qDebug() << "Refreshing repo" << alias << "with" << t << "method repoSetData()";
        t->repoSetData(alias, QStringLiteral("refresh-now"), QStringLiteral("true"));
    }
    case DisableRepo:
        this->pFetchRepoPackages(alias);
        this->getAllPackages();
        break;
    default:
        this->getAllPackages();
        break;
    }
}

void OrnZypp::onPackage(PackageKit::Transaction::Info info,
                        const QString &packageId,
                        const QString &summary)
{
    Q_UNUSED(summary)
    auto idParts = packageId.split(QChar(';'));
    auto name = idParts.first();
    auto repo = idParts.last();
    switch (info)
    {
    case PackageKit::Transaction::InfoInstalled:
        // Add all installed packages to be available show "installed" status
        // for packages that were installed not from OpenRepos
        mInstalledPackages.insert(name, packageId);
        break;
    case PackageKit::Transaction::InfoAvailable:
        if (repo.startsWith(repoNamePrefix))
        {
            // Add package name to available packages if
            // it's from an ORN repo or is installed
            mAvailablePackages.insertMulti(name, packageId);
        }
        else if (repo == installed)
        {
            repo = this->packageRepo(name);
            if (!repo.isEmpty())
            {
                idParts.replace(3, repo);
                mAvailablePackages.insert(name, idParts.join(';'));
            }
        }
        break;
    case PackageKit::Transaction::InfoEnhancement:
        // Stupid PackageKit cannot refresh updates list after a repo was removed!
        // So we need to manually filter updates
        repo = this->packageRepo(name);
        if (repo.startsWith(repoNamePrefix))
        {
            mUpdates.insert(name, packageId);
        }
        break;
    default:
        break;
    }
}

void OrnZypp::pPrepareFetching(PackageKit::Transaction *&transaction)
{
    if (transaction)
    {
        qWarning() << "OrnZypp is already fetching packages - canceling";
        transaction->cancel();
        transaction->deleteLater();
        transaction = 0;
    }
    transaction = Orn::transaction();
    connect(transaction, &PackageKit::Transaction::package, this, &OrnZypp::onPackage);
}

void OrnZypp::pInstalledApps()
{
    AppList apps;

    if (mInstalledPackages.isEmpty() ||
        mRepos.isEmpty())
    {
        qWarning() << "Installed packages or repositories list is empty";
        emit this->installedAppsReady(apps);
        return;
    }

    // Prepare vars for parsing desktop files
    QString nameKey(QStringLiteral("Desktop Entry/Name"));
    auto trNameKey = QString(nameKey).append("[%0]");
    auto localeName = QLocale::system().name();
    auto localeNameKey = trNameKey.arg(localeName);
    QString langNameKey;
    if (localeName.length() > 2)
    {
        langNameKey = trNameKey.arg(localeName.left(2));
    }
    QString iconKey(QStringLiteral("Desktop Entry/Icon"));
    QString iconPath(QStringLiteral("/usr/share/icons/hicolor/%0/apps/%1.png"));
    QStringList iconSizes{
        QStringLiteral("86x86"),
        QStringLiteral("108x108"),
        QStringLiteral("128x128"),
        QStringLiteral("256x256")
    };

    // Prepare set to filter installed apps to show only those from OpenRepos
    QSet<QString> ornPackages;
    for (const auto &repo : mRepos)
    {
        for (const auto &package : repo.packages)
        {
            ornPackages.insert(package);
        }
    }

    for (const auto &id : mInstalledPackages.values())
    {
        auto idParts = id.split(QChar(';'));
        auto name = idParts[0];

        // Actual filtering
        if (!ornPackages.contains(name))
        {
            continue;
        }

        auto title = name;
        QString icon;
        auto desktopFiles = PackageKit::Transaction::packageDesktopFiles(name);

        qDebug() << "Adding installed package" << name;

        if (!desktopFiles.isEmpty())
        {
            auto desktopFile = desktopFiles.first();
            qDebug() << "Parsing desktop file" << desktopFile;
            QSettings desktop(desktopFile, QSettings::IniFormat);
            desktop.setIniCodec("UTF-8");
            // Read pretty name
            if (desktop.contains(localeNameKey))
            {
                title = desktop.value(localeNameKey).toString();
            }
            else if (!langNameKey.isEmpty() && desktop.contains(langNameKey))
            {
                title = desktop.value(langNameKey).toString();
            }
            else if (desktop.contains(nameKey))
            {
                title = desktop.value(nameKey).toString();
            }
            qDebug() << "Using name" << title << "for package" << name;
            // Find icon
            if (desktop.contains(iconKey))
            {
                auto iconName = desktop.value(iconKey).toString();
                for (const auto &s : iconSizes)
                {
                    auto pi = iconPath.arg(s, iconName);
                    if (QFileInfo(pi).isFile())
                    {
                        qDebug() << "Using package icon" << pi;
                        icon = pi;
                        break;
                    }
                }
            }
        }
        apps << App{
            name,
            title,
            idParts[1],
            this->packageRepo(name).mid(repoNamePrefixLength),
            icon,
            // The update package id or an empty string
            mUpdates.value(name)
        };
    }

    emit this->installedAppsReady(apps);
    mBusy = false;
}

void OrnZypp::pFetchRepos()
{
    emit this->beginRepoFetching();

    qDebug() << "Refreshing repo list";

    mRepos.clear();

    // NOTE: A hack for SSU repos. Can break on ssu config changes.
    QSettings ssuSettings(SSU_CONFIG_PATH, QSettings::IniFormat);
    auto disabled = ssuSettings.value(SSU_DISABLED_KEY).toStringList().toSet();
    ssuSettings.beginGroup(SSU_REPOS_GROUP);
    auto aliases = ssuSettings.childKeys();

    // NOTE: A hack for packages filtering
    for (const auto &alias : aliases)
    {
        if (alias.startsWith(repoNamePrefix))
        {
            auto enabled = !disabled.contains(alias);
            qDebug() << "Found " << (enabled ? "enabled" : "disabled") << " repo" << alias;
            mRepos.insert(alias, RepoMeta{ enabled, QSet<QString>() });
            this->pFetchRepoPackages(alias);
        }
    }

    emit this->endRepoFetching();
}

void OrnZypp::pFetchRepoPackages(const QString &alias)
{
    if (!mRepos.contains(alias))
    {
        qWarning() << "Repo list does not contain the" << alias << "repo";
        return;
    }

    auto &repo = mRepos[alias];

    qDebug() << "Clearing repo" << alias << "package list";
    repo.packages.clear();

    auto spath = solvTmpl.arg(alias);
    qDebug() << "Reading" << spath;
    auto spool = pool_create();
    auto srepo = repo_create(spool, alias.toUtf8().data());
    auto sfile = fopen(spath.toUtf8().data(), "r");

    repo_add_solv(srepo, sfile, 0);
    fclose(sfile);

    for (int i = 0; i < spool->nsolvables; ++i)
    {
        auto s = &spool->solvables[i];
        repo.packages.insert(solvable_lookup_str(s, SOLVABLE_NAME));
    }

    repo_free(srepo, 0);
    pool_free(spool);

    qDebug() << repo.packages.size() << "packages were added to the" << alias << "repo";
}

QDBusPendingCallWatcher *OrnZypp::pDbusCall(const QString &method, const QVariantList &args)
{
    auto call = QDBusMessage::createMethodCall(ssuInterface, ssuPath, ssuInterface, method);
    if (!args.empty())
    {
        call.setArguments(args);
    }
    qDebug() << "Calling" << call;
    auto pCall = QDBusConnection::systemBus().asyncCall(call);
    auto watcher = new QDBusPendingCallWatcher(pCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
#ifdef QT_DEBUG
            [watcher]()
            {
                if (watcher->isError())
                {
                    auto e = watcher->error();
                    qCritical() << e.name() << e.message();
                }
                watcher->deleteLater();
            }
#else
            watcher, &QDBusPendingCallWatcher::deleteLater
#endif
            );
    return watcher;
}
