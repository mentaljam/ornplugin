#ifndef ORNAPPLICATION_H
#define ORNAPPLICATION_H

#include <PackageKit/packagekit-qt5/Transaction>

#include "ornapirequest.h"
#include <QDateTime>

class OrnApplication : public OrnApiRequest
{
    Q_OBJECT

    Q_PROPERTY(bool canBeLaunched READ canBeLaunched NOTIFY canBeLaunchedChanged)
    Q_PROPERTY(bool updateAvailable MEMBER mUpdateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(RepoStatus repoStatus MEMBER mRepoStatus NOTIFY repoStatusChanged)
    Q_PROPERTY(QString installedVersion MEMBER mInstalledVersion NOTIFY installedVersionChanged)
    Q_PROPERTY(QString availableVersion MEMBER mAvailableVersion NOTIFY availableVersionChanged)

    Q_PROPERTY(quint32 appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(quint32 userId MEMBER mUserId NOTIFY updated)
    Q_PROPERTY(quint32 ratingCount MEMBER mRatingCount NOTIFY updated)
    Q_PROPERTY(quint32 commentsCount MEMBER mCommentsCount NOTIFY updated)
    Q_PROPERTY(quint32 downloadsCount MEMBER mDownloadsCount NOTIFY updated)
    Q_PROPERTY(float rating MEMBER mRating NOTIFY updated)
    Q_PROPERTY(QString title MEMBER mTitle NOTIFY updated)
    Q_PROPERTY(QString userName MEMBER mUserName NOTIFY updated)
    Q_PROPERTY(QString userIconSource MEMBER mUserIconSource NOTIFY updated)
    Q_PROPERTY(QString iconSource MEMBER mIconSource NOTIFY updated)
    Q_PROPERTY(QString packageName MEMBER mPackageName NOTIFY updated)
    Q_PROPERTY(QString body MEMBER mBody NOTIFY updated)
    Q_PROPERTY(QString changelog MEMBER mChangelog NOTIFY updated)
    Q_PROPERTY(QDateTime created MEMBER mCreated NOTIFY updated)
    Q_PROPERTY(QDateTime updated MEMBER mUpdated NOTIFY updated)
    Q_PROPERTY(QVariantList screenshots MEMBER mScreenshots NOTIFY updated)

public:

    enum RepoStatus
    {
        RepoNotInstalled,
        RepoDisabled,
        RepoEnabled
    };
    Q_ENUM(RepoStatus)

    explicit OrnApplication(QObject *parent = 0);

    quint32 appId() const;
    void setAppId(const quint32 &appId);

    bool canBeLaunched() const;

signals:
    void canBeLaunchedChanged();
    void installedVersionChanged();
    void availableVersionChanged();

    void repoStatusChanged();

    void networkManagerChanged();
    void errorInstallRepo();

    void appIdChanged();
    void appNotFound();
    void updateError();
    void updated();
    void installed();
    void removed();
    void updateAvailableChanged();

public slots:
    void update();
    void enableRepo();
    void install();
    void remove();
    void launch();

private slots:
    void onRepoListChanged();
    void checkAppPackage(const PackageKit::Transaction::Filter &filter);
    void onRepoStatusChanged();
    void checkRepoUpdate(const QString &repoId, const QString &description, bool enabled);
    void onPackage(PackageKit::Transaction::Info info, const QString &packageID, const QString &summary);
    void onInstalled(const QString &packageId, const QString &version);
    void checkUpdates();
    void onJsonREady(const QJsonDocument &jsonDoc);

private:
    bool mUpdateAvailable;

    RepoStatus mRepoStatus;
    QString mRepoId;

    QString mInstalledPackageId;
    QString mAvailablePackageId;
    QString mDesktopFile;

    QString mInstalledVersion;
    QString mAvailableVersion;

    quint32 mAppId;
    quint32 mUserId;
    quint32 mRatingCount;
    quint32 mCommentsCount;
    quint32 mDownloadsCount;
    float mRating;
    QString mTitle;
    QString mUserName;
    QString mUserIconSource;
    QString mIconSource;
    QString mPackageName;
    QString mBody;
    QString mChangelog;
    QDateTime mCreated;
    QDateTime mUpdated;    
    QList<quint32> mTagsIds;
    QList<quint32> mCategoryIds;
    /// A list of maps with keys [ url, thumb ]
    QVariantList mScreenshots;
};

#endif // ORNAPPLICATION_H
