#ifndef ORNZYPP_H
#define ORNZYPP_H

#include <QObject>

#include <PackageKit/packagekit-qt5/Transaction>

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingReply>
#include <QDebug>

class OrnZypp : public QObject
{
    Q_OBJECT

public:

    /// Ssu actions
    enum RepoAction {
        RemoveRepo  = 0,
        AddRepo     = 1,
        DisableRepo = 2,
        EnableRepo  = 3,
    };
    Q_ENUM(RepoAction)

    enum RepoStatus
    {
        RepoNotInstalled,
        RepoDisabled,
        RepoEnabled
    };
    Q_ENUM(RepoStatus)

    struct App
    {
        QString name;
        QString title;
        QString version;
        QString author;
        QString icon;
    };

    struct Repo
    {
        bool enabled;
        QString alias;
        QString author;
    };

    typedef QList<App> AppList;
    typedef QList<Repo> RepoList;

    static const QString ssuInterface;
    static const QString ssuPath;
    static const QString ssuModifyRepo;
    static const QString ssuAddRepo;
    static const QString repoBaseUrl;
    static const QString repoNamePrefix;
    static const QString installed;
    static const int repoNamePrefixLength;

public:
    explicit OrnZypp(QObject *parent = 0);

    static OrnZypp *instance();

    RepoStatus repoStatus(const QString &alias) const;
    RepoList repoList() const;

    bool isAvailable(const QString &packageName) const;
    QStringList availablePackages(const QString &packageName) const;

    bool isInstalled(const QString &packageName) const;
    QString installedPackage(const QString &packageName) const;

    bool hasUpdate(const QString &packageName) const;
    QString updatePackage(const QString &packageName) const;

    Q_INVOKABLE static QString deviceModel();    

signals:
    void installedAppsReady(const AppList &apps);
    void reposFetched();
    void repoModified(const QString &alias, const RepoAction &action);

    void availablePackagesChanged();
    void installedPackagesChanged();
    void updatesChanged();

    void packageInstalled(const QString &packageId);
    void packageRemoved(const QString &packageId);

public slots:
    bool getInstalledApps();

    void addRepo(const QString &author);
    void modifyRepo(const QString &alias, const RepoAction &action);
    void enableRepos(bool enable);

    void refreshRepos(bool force = false);
    void refreshRepo(const QString &alias, bool force = false);

    void getAvailablePackages();
    void getInstalledPackages();
    void getUpdates();
    void getAllPackages();

    void installPackage(const QString &packageId);
    // TODO: an option to autoRemove?
    void removePackage(const QString &packageId);

private slots:
    void fetchRepos();
    void onRepoModified(const QString &alias, const RepoAction &action);
    void onPackage(PackageKit::Transaction::Info info,
                   const QString &packageId,
                   const QString &summary);

private:
    void prepareFetching(PackageKit::Transaction *&transaction);

    void pInstalledApps();

    template<typename Func>
    void pDbusCall(const QString &method, Func returnMethod,
                   const QVariantList &args = QVariantList())
    {
        auto call = QDBusMessage::createMethodCall(ssuInterface, ssuPath, ssuInterface, method);
        if (!args.empty())
        {
            call.setArguments(args);
        }
        qDebug() << "Calling" << call;
        auto pCall = QDBusConnection::systemBus().asyncCall(call);
        auto watcher = new QDBusPendingCallWatcher(pCall, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, returnMethod);
        connect(watcher, &QDBusPendingCallWatcher::finished,
#ifdef QT_DEBUG
                [this, watcher]()
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
    }

private:
    bool mBusy;
    PackageKit::Transaction *mAvailableFetcher;
    PackageKit::Transaction *mInstalledFetcher;
    PackageKit::Transaction *mUpdatesFetcher;
    /// { alias, enabled }
    QHash<QString, bool> mRepos;
    /// { name, package_id }
    QHash<QString, QString> mInstalledPackages;
    /// { name, package_id }
    QHash<QString, QString> mUpdates;
    /// It's really a multihash { name, package_id_list }
    QMultiHash<QString, QString> mAvailablePackages;

    static OrnZypp *gInstance;
};

#endif // ORNZYPP_H
