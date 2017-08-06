#include "ornzypp.h"
#include "orn.h"

#include <PackageKit/packagekit-qt5/Daemon>

#include <zypp/RepoManager.h>
#include <zypp/ZYppFactory.h>
#include <zypp/target/rpm/RpmDb.h>

#include <QtConcurrent/QtConcurrent>
#include <QSettings>
#include <QLocale>
#include <QFileInfo>

#define SSU_METHOD_DISPLAYNAME QStringLiteral("displayName")
#define SSU_CONFIG_PATH        QStringLiteral("/etc/ssu/ssu.ini")
#define SSU_REPOS_GROUP        QStringLiteral("repository-urls")
#define SSU_DISABLED_KEY       QStringLiteral("disabled-repos")

const QString OrnZypp::ssuInterface(QStringLiteral("org.nemo.ssu"));
const QString OrnZypp::ssuPath(QStringLiteral("/org/nemo/ssu"));
const QString OrnZypp::ssuModifyRepo(QStringLiteral("modifyRepo"));
const QString OrnZypp::ssuAddRepo(QStringLiteral("addRepo"));
const QString OrnZypp::repoBaseUrl(QStringLiteral("https://sailfish.openrepos.net/%0/personal/main"));
const QString OrnZypp::repoNamePrefix(QStringLiteral("openrepos-"));
const QString OrnZypp::installed(QStringLiteral("installed"));
const int OrnZypp::repoNamePrefixLength = OrnZypp::repoNamePrefix.length();
OrnZypp *OrnZypp::gInstance = 0;

OrnZypp::OrnZypp(QObject *parent) :
    QObject(parent),
    mBusy(false),
    mAvailableFetcher(0),
    mInstalledFetcher(0),
    mUpdatesFetcher(0)
{
    // Initialize SSU target
    zypp::ZYppFactory::instance().getZYpp()->initializeTarget("/");

    // Refetch available packages if repos have been changed
    connect(this, &OrnZypp::repoModified, this, &OrnZypp::onRepoModified);

    // Fetch repos
    QtConcurrent::run(this, &OrnZypp::fetchRepos);
    // Fetch packages
    this->getAllPackages();
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
    return mRepos[alias] ? RepoEnabled : RepoDisabled;
}

OrnZypp::RepoList OrnZypp::repoList() const
{
    RepoList repos;
    for (auto it = mRepos.constBegin(); it != mRepos.constEnd(); ++it)
    {
        auto alias = it.key();
        repos << Repo{ it.value(), alias, alias.mid(repoNamePrefixLength) };
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

bool OrnZypp::hasUpdate(const QString &packageName) const
{
    return mUpdates.contains(packageName);
}

QString OrnZypp::updatePackage(const QString &packageName) const
{
    return mUpdates.value(packageName);
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

void OrnZypp::addRepo(const QString &author)
{
    auto alias = repoNamePrefix + author;
    pDbusCall(ssuAddRepo, [this, alias]()
        {
            qDebug() << "Repo" << alias << "have been added";
            mRepos.insert(alias, true);
            emit this->repoModified(alias, AddRepo);
        },
        QVariantList{ alias, repoBaseUrl.arg(author) });
}

void OrnZypp::modifyRepo(const QString &alias, const OrnZypp::RepoAction &action)
{
    pDbusCall(ssuModifyRepo, [this, alias, action]()
        {
            qDebug() << "Repo" << alias << "have been modified with" << action;
            switch (action)
            {
            case RemoveRepo:
                mRepos.remove(alias);
                break;
            case DisableRepo:
                mRepos[alias] = false;
                break;
            case EnableRepo:
                mRepos[alias] = true;
                break;
            default:
                break;
            }
            emit this->repoModified(alias, action);
        },
        QVariantList{ action, alias });
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
    this->prepareFetching(mAvailableFetcher);
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
    this->prepareFetching(mInstalledFetcher);
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
    this->prepareFetching(mUpdatesFetcher);
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
        mInstalledPackages.insert(
                    PackageKit::Transaction::packageName(packageId),
                    packageId);
        emit this->installedPackagesChanged();
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

void OrnZypp::fetchRepos()
{
    mRepos.clear();
    // NOTE: A hack for SSU repos. Can break on ssu config changes.
    QSettings ssuSettings(SSU_CONFIG_PATH, QSettings::IniFormat);
    auto disabled = ssuSettings.value(SSU_DISABLED_KEY).toStringList().toSet();
    ssuSettings.beginGroup(SSU_REPOS_GROUP);
    auto repos = ssuSettings.childKeys();
    for (const auto &repo : repos)
    {
        if (repo.startsWith(repoNamePrefix))
        {
            auto enabled = !disabled.contains(repo);
            qDebug() << "Found repo { alias:" << repo << ", enabled:" << enabled << "}";
            mRepos.insert(repo, enabled);
        }
    }
    emit this->reposFetched();
}

void OrnZypp::onRepoModified(const QString &alias, const RepoAction &action)
{
    // TODO: Currently avaliable apps are reloaded on every repo change
    if (action == AddRepo || action == EnableRepo)
    {
        auto t = Orn::transaction();
        connect(t, &PackageKit::Transaction::finished, this, &OrnZypp::getAllPackages);
        qDebug() << "Refreshing repo" << alias << "with" << t << "method repoSetData()";
        t->repoSetData(alias, QStringLiteral("refresh-now"), QStringLiteral("true"));
    }
    else
    {
        this->getAllPackages();
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
    // Installed packages don't contain repo in ID so
    // add all installed packages to lists
    switch (info)
    {
    case PackageKit::Transaction::InfoInstalled:
        mInstalledPackages.insert(name, packageId);
        break;
    case PackageKit::Transaction::InfoAvailable:
        if (repo.startsWith(repoNamePrefix) || repo == installed)
        {
            // Add package name to available packages if
            // it's from an ORN repo or is installed
            mAvailablePackages.insertMulti(name, packageId);
        }
        break;
    case PackageKit::Transaction::InfoEnhancement:
        if (repo.startsWith(repoNamePrefix))
        {
            mUpdates.insert(name, packageId);
        }
        break;
    default:
        break;
    }
}

void OrnZypp::prepareFetching(PackageKit::Transaction *&transaction)
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
    // Load ORN repositories
    zypp::RepoManager manager;
    auto repos = manager.knownRepositories();
    for (const zypp::RepoInfo &repo : repos)
    {
        auto alias = repo.alias();
        auto qalias = QString::fromStdString(alias);
        if (qalias.startsWith(repoNamePrefix))
        {
            qDebug() << "Loading cache for" << qalias;
            manager.loadFromCache(repo);
        }
    }

    // Load zypp target
    zypp::ZYpp &zypp = *zypp::ZYppFactory::instance().getZYpp();
    zypp.target()->reload();

    // Prepare vars for parsing desktop files
    QString desktop(QStringLiteral(".desktop"));
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

    // List installed packages
    const auto &proxy = zypp.pool().proxy();
    const auto &rpmDb = zypp.target()->rpmDb();
    AppList apps;
    for (auto it = proxy.byKindBegin(zypp::ResKind::package);
         it != proxy.byKindEnd(zypp::ResKind::package);
         ++it)
    {
        zypp::ui::Selectable::constPtr sel(*it);
        for (auto it2 = sel->picklistBegin(); it2 != sel->picklistEnd(); ++it2)
        {
            zypp::PoolItem pi(*it2);
            auto solv = pi.satSolvable();
            auto repo = solv.repository().info().alias();
            if (repo != "@System" && sel->identicalInstalled(pi))
            {
                qDebug() << "Adding installed package" << QString::fromStdString(solv.asString());
                // Find .desktop file
                zypp::target::rpm::RpmHeader::constPtr header;
                auto name = solv.name();
                auto qname = QString::fromStdString(name);
                auto qtitle = qname;
                auto edition = solv.edition();
                QString qicon;
                rpmDb.getData(name, edition, header);
                for (const auto &f : header->tag_filenames())
                {
                    auto qf = QString::fromStdString(f);
                    if (qf.endsWith(desktop))
                    {
                        qDebug() << "Parsing desktop file" << qf;
                        QSettings desktopFile(qf, QSettings::IniFormat);
                        desktopFile.setIniCodec("UTF-8");
                        // Read pretty name
                        if (desktopFile.contains(localeNameKey))
                        {
                            qtitle = desktopFile.value(localeNameKey).toString();
                        }
                        else if (!langNameKey.isEmpty() && desktopFile.contains(langNameKey))
                        {
                            qtitle = desktopFile.value(langNameKey).toString();
                        }
                        else if (desktopFile.contains(nameKey))
                        {
                            qtitle = desktopFile.value(nameKey).toString();
                        }
                        qDebug() << "Using name" << qtitle << "for package" << qname;
                        // Find icon
                        if (desktopFile.contains(iconKey))
                        {
                            for (const auto &s : iconSizes)
                            {
                                auto qi = iconPath.arg(s, desktopFile.value(iconKey).toString());
                                if (QFileInfo(qi).isFile())
                                {
                                    qDebug() << "Using package icon" << qi;
                                    qicon = qi;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                apps << App{
                    qname,
                    qtitle,
                    QString::fromStdString(edition.asString()),
                    QString::fromStdString(repo).mid(repoNamePrefixLength),
                    qicon
                };
            }
        }
    }

    emit this->installedAppsReady(apps);
    mBusy = false;
}
