#include "ornzypp.h"
#include "orn.h"

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
    mFetchingPackages(false)
{
    // Initialize SSU target
    zypp::ZYppFactory::instance().getZYpp()->initializeTarget("/");

    // Refetch available packages if repos have been changed
    connect(this, &OrnZypp::repoModified, this, &OrnZypp::onRepoModified);
    // Refetch installed packages if available have been changed
    connect(this, &OrnZypp::availablePackagesChanged, this, &OrnZypp::fetchInstalledPackages);

    // Fetch repos
    QtConcurrent::run(this, &OrnZypp::fetchRepos);
    // Fetch available packages
    this->fetchAvailablePackages();
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
    return mAvailablePackages.values(packageName);
}

bool OrnZypp::isInstalled(const QString &packageName) const
{
    return mInstalledPackages.contains(packageName);
}

QString OrnZypp::installedPackage(const QString &packageName) const
{
    return mInstalledPackages.value(packageName);
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
        connect(t, &PackageKit::Transaction::finished, this, &OrnZypp::fetchAvailablePackages);
        qDebug() << "Refreshing repo" << alias << "with" << t << "method repoSetData()";
        t->repoSetData(alias, QStringLiteral("refresh-now"), QStringLiteral("true"));
    }
    else
    {
        this->fetchAvailablePackages();
    }
}

void OrnZypp::fetchAvailablePackages()
{
    // Get available packages and got installed when finished
    if (mFetchingPackages)
    {
        qWarning() << "OrnZypp is already fetching packages";
        return;
    }
    mFetchingPackages = true;

    auto t = Orn::transaction();
    connect(t, &PackageKit::Transaction::finished, this, &OrnZypp::availablePackagesChanged);
    connect(t, &PackageKit::Transaction::package, this, &OrnZypp::onAvailablePackage);
    qDebug() << "Getting all available packages with" << t << "method getPackages()";

    if (!mAvailablePackages.isEmpty())
    {
        mAvailablePackages.clear();
    }

    t->getPackages(PackageKit::Transaction::FilterSupported);
}

void OrnZypp::fetchInstalledPackages()
{
    if (mAvailablePackages.isEmpty())
    {
        mFetchingPackages = false;
        return;
    }

    auto t = Orn::transaction();
    connect(t, &PackageKit::Transaction::finished, this, &OrnZypp::installedPackagesChanged);
    connect(t, &PackageKit::Transaction::finished, [this](){ mFetchingPackages = false; });
    connect(t, &PackageKit::Transaction::package, this, &OrnZypp::onInstalledPackage);
    qDebug() << "Getting all installed packages with" << t << "method getPackages(FilterInstalled)";

    if (!mInstalledPackages.isEmpty())
    {
        mInstalledPackages.clear();
    }

    t->getPackages(PackageKit::Transaction::FilterInstalled);
}

void OrnZypp::onAvailablePackage(PackageKit::Transaction::Info info,
                                 const QString &packageId,
                                 const QString &summary)
{
    Q_UNUSED(info)
    Q_UNUSED(summary)
    auto id = packageId.split(QChar(';'));
    auto repo = id.last();
    // NOTE: It seems there is no way to get a full list of packages available from repos
    // because the installed one does not have a repo alias in ID
    if (repo.startsWith(repoNamePrefix) || repo == installed)
    {
        // Add package name to available packages if it's from an ORN repo
        mAvailablePackages.insertMulti(id[0], packageId);
    }
}

void OrnZypp::onInstalledPackage(PackageKit::Transaction::Info info,
                                 const QString &packageId,
                                 const QString &summary)
{
    Q_UNUSED(info)
    Q_UNUSED(summary)
    auto name = PackageKit::Transaction::packageName(packageId);
    if (mAvailablePackages.contains(name))
    {
        // If an installed package is also among available then
        // it's can be considered as installed from ORN
        mInstalledPackages.insert(name, packageId);
    }
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
                        qDebug() << "Using package name" << qname;
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
