#ifndef ORNAPPLICATION_H
#define ORNAPPLICATION_H

#include "ornapirequest.h"
#include "ornzypp.h"

#include <QDateTime>

class OrnApplication : public OrnApiRequest
{
    friend class OrnBookmarksModel;

    Q_OBJECT

    Q_PROPERTY(bool canBeLaunched READ canBeLaunched NOTIFY canBeLaunchedChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(OrnZypp::RepoStatus repoStatus MEMBER mRepoStatus NOTIFY repoStatusChanged)
    Q_PROPERTY(QString installedVersion MEMBER mInstalledVersion NOTIFY installedVersionChanged)
    Q_PROPERTY(QString availableVersion MEMBER mAvailableVersion NOTIFY availableVersionChanged)
    Q_PROPERTY(QString repoAlias MEMBER mRepoAlias NOTIFY updated)

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
    Q_PROPERTY(QString category READ category NOTIFY updated)
    Q_PROPERTY(QDateTime created MEMBER mCreated NOTIFY updated)
    Q_PROPERTY(QDateTime updated MEMBER mUpdated NOTIFY updated)
    Q_PROPERTY(QVariantList categories MEMBER mCategories NOTIFY updated)
    Q_PROPERTY(QVariantList screenshots MEMBER mScreenshots NOTIFY updated)

public:
    explicit OrnApplication(QObject *parent = 0);

    quint32 appId() const;
    void setAppId(const quint32 &appId);

    bool updateAvailable() const;

    bool canBeLaunched() const;

    QString category() const;

signals:
    void appIdChanged();
    void canBeLaunchedChanged();
    void updated();
    void availableVersionChanged();
    void installedVersionChanged();
    void updateAvailableChanged();
    void repoStatusChanged();
    void appNotFound();
    // TODO: currently does nothing
    void updateError();
    void installed();
    void removed();

public slots:
    void update();
    void install();
    void remove();
    void launch();

private slots:
    void onJsonReady(const QJsonDocument &jsonDoc);
    void onReposChanged();
    void onAvailablePackagesChanged();
    void onInstalledPackagesChanged();
    // This two slots need only for emitting signals
    void onPackageInstalled(const QString &packageId);
    void onPackageRemoved(const QString &packageId);

private:
    void checkDesktopFile();

private:
    OrnZypp::RepoStatus mRepoStatus;
    QString mRepoAlias;

    QString mInstalledVersion;
    QString mInstalledPackageId;
    QString mAvailableVersion;
    QString mAvailablePackageId;

    QString mDesktopFile;

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
    /// A list of maps with keys [ id, name ]
//    IdNameList mTags;
    /// A list of maps with keys [ id, name ]
    QVariantList mCategories;
    /// A list of maps with keys [ url, thumb ]
    QVariantList mScreenshots;
};

#endif // ORNAPPLICATION_H
