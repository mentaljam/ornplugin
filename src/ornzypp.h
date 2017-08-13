#ifndef ORNZYPP_H
#define ORNZYPP_H

#include <QObject>
#include <QSet>

#include <PackageKit/packagekit-qt5/Transaction>

class QDBusPendingCallWatcher;

class OrnZypp : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool updatesAvailable READ updatesAvailable NOTIFY updatesChanged)

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

    Q_ENUM(PackageKit::Transaction::Error)

    struct App
    {
        QString name;
        QString title;
        QString version;
        QString author;
        QString icon;
        QString updateId;
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
    static const QString ssuUpdateRepos;
    static const QString repoBaseUrl;
    static const QString repoNamePrefix;
    static const QString installed;
    static const QString solvTmpl;
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

    bool updatesAvailable() const;
    bool hasUpdate(const QString &packageName) const;
    Q_INVOKABLE QString updatePackage(const QString &packageName) const;

    QString packageRepo(const QString &name) const;

    Q_INVOKABLE static QString deviceModel();    

signals:
    void installedAppsReady(const AppList &apps);
    void beginRepoFetching();
    void endRepoFetching();
    void repoModified(const QString &alias, const RepoAction &action);

    void availablePackagesChanged();
    void installedPackagesChanged();
    void updatesChanged();

    void packageInstalled(const QString &packageId);
    void packageRemoved(const QString &packageId);

    void pkError(int error, const QString &details);

public slots:
    bool getInstalledApps();

    void fetchRepos();

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
    void updateAll();

private slots:
    void onRepoModified(const QString &alias, const RepoAction &action);
    void onPackage(PackageKit::Transaction::Info info,
                   const QString &packageId,
                   const QString &summary);

private:    
    PackageKit::Transaction *transaction();
    void pPrepareFetching(PackageKit::Transaction *&fetcher);
    void pInstalledApps();
    /// This method is one big dirty hack...
    void pFetchRepos();
    void pFetchRepoPackages(const QString &alias);
    QDBusPendingCallWatcher *pDbusCall(const QString &method, const QVariantList &args = QVariantList());

private:
    struct RepoMeta
    {
        bool enabled;
        QSet<QString> packages;
    };

    bool mBusy;
    PackageKit::Transaction *mAvailableFetcher;
    PackageKit::Transaction *mInstalledFetcher;
    PackageKit::Transaction *mUpdatesFetcher;
    QHash<QString, RepoMeta> mRepos;
    /// { name, package_id }
    QHash<QString, QString> mInstalledPackages;
    /// { name, package_id }
    QHash<QString, QString> mUpdates;
    /// It's really a multihash { name, package_id_list }
    QMultiHash<QString, QString> mAvailablePackages;

    static OrnZypp *gInstance;
};

#endif // ORNZYPP_H
