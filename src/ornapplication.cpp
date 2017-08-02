#include "ornapplication.h"
#include "orn.h"
#include "ornversion.h"
#include "orncategorylistitem.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>

#include <QDebug>

OrnApplication::OrnApplication(QObject *parent) :
    OrnApiRequest(parent),
    mUpdateAvailable(false),
    mRepoStatus(OrnZypp::RepoNotInstalled),
    mAppId(0),
    mUserId(0),
    mRatingCount(0),
    mCommentsCount(0),
    mRating(0.0)
{
    connect(this, &OrnApplication::jsonReady, this, &OrnApplication::onJsonReady);

    auto ornZypp = OrnZypp::instance();
    connect(ornZypp, &OrnZypp::reposFetched, this, &OrnApplication::onReposChanged);
    connect(ornZypp, &OrnZypp::repoModified, this, &OrnApplication::onReposChanged);
    connect(ornZypp, &OrnZypp::availablePackagesChanged, this, &OrnApplication::onAvailablePackagesChanged);
    connect(ornZypp, &OrnZypp::installedPackagesChanged, this, &OrnApplication::onInstalledPackagesChanged);

    connect(this, &OrnApplication::installedVersionChanged, this, &OrnApplication::checkUpdates);
    connect(this, &OrnApplication::availableVersionChanged, this, &OrnApplication::checkUpdates);
}

quint32 OrnApplication::appId() const
{
    return mAppId;
}

void OrnApplication::setAppId(const quint32 &appId)
{
    if (mAppId != appId)
    {
        mAppId = appId;
        emit this->appIdChanged();
    }
}

bool OrnApplication::canBeLaunched() const
{
    return !mDesktopFile.isEmpty();
}

void OrnApplication::update()
{
    auto url = OrnApiRequest::apiUrl(QStringLiteral("apps/%0").arg(mAppId));
    auto request = OrnApiRequest::networkRequest();
    request.setUrl(url);
    this->run(request);
}

void OrnApplication::install()
{
    if (!mAvailablePackageId.isEmpty())
    {
        OrnZypp::instance()->installPackage(mAvailablePackageId);
    }
}

void OrnApplication::remove()
{
    if (!mInstalledPackageId.isEmpty())
    {
        OrnZypp::instance()->removePackage(mAvailablePackageId);
    }
}

void OrnApplication::launch()
{
    if (mDesktopFile.isEmpty())
    {
        qDebug() << "Application" << mPackageName << "could not be launched";
        return;
    }
    qDebug() << "Launching" << mDesktopFile;
    QDesktopServices::openUrl(QUrl::fromLocalFile(mDesktopFile));
}

void OrnApplication::onJsonReady(const QJsonDocument &jsonDoc)
{
    auto jsonObject = jsonDoc.object();
    QString urlKey(QStringLiteral("url"));
    QString nameKey(QStringLiteral("name"));

    mCommentsCount = Orn::toUint(jsonObject[QStringLiteral("comments_count")]);
    mDownloadsCount = Orn::toUint(jsonObject[QStringLiteral("downloads")]);
    mTitle = Orn::toString(jsonObject[QStringLiteral("title")]);
    mIconSource = Orn::toString(jsonObject[QStringLiteral("icon")].toObject()[urlKey]);
    mPackageName = Orn::toString(jsonObject[QStringLiteral("package")].toObject()[nameKey]);
    mBody = Orn::toString(jsonObject[QStringLiteral("body")]);
    mChangelog = Orn::toString(jsonObject[QStringLiteral("changelog")]);
    mCreated = Orn::toDateTime(jsonObject[QStringLiteral("created")]);
    mUpdated = Orn::toDateTime(jsonObject[QStringLiteral("updated")]);

    auto userObject = jsonObject[QStringLiteral("user")].toObject();
    mUserId = Orn::toUint(userObject[QStringLiteral("uid")]);
    mUserName = Orn::toString(userObject[nameKey]);
    mUserIconSource = Orn::toString(userObject[QStringLiteral("picture")].toObject()[urlKey]);

    QString ratingKey(QStringLiteral("rating"));
    auto ratingObject = jsonObject[ratingKey].toObject();
    mRatingCount = Orn::toUint(ratingObject[QStringLiteral("count")]);
    mRating = ratingObject[ratingKey].toString().toFloat();

    mTagsIds = Orn::toIntList(jsonObject[QStringLiteral("tags")]);
    mCategoryIds = Orn::toIntList(jsonObject[QStringLiteral("category")]);
    mCategory = OrnCategoryListItem::categoryName(mCategoryIds.last());

    QString thumbsKey(QStringLiteral("thumbs"));
    QString largeKey(QStringLiteral("large"));
    auto jsonArray = jsonObject[QStringLiteral("screenshots")].toArray();
    mScreenshots.clear();
    for (const QJsonValue &v: jsonArray)
    {
        auto o = v.toObject();
        mScreenshots << QVariantMap{
            { "url",   Orn::toString(o[urlKey]) },
            { "thumb", Orn::toString(o[thumbsKey].toObject()[largeKey]) }
        };
    }

    if (!mUserName.isEmpty())
    {
        // Generate repository name
        mRepoAlias = OrnZypp::repoNamePrefix + mUserName;
        // Check if repository is enabled
        this->onReposChanged();
    }
    else
    {
        mRepoAlias.clear();
    }

    qDebug() << "Application" << mPackageName << "information updated";
    emit this->updated();
}

void OrnApplication::onReposChanged()
{
    if (mRepoAlias.isEmpty())
    {
        return;
    }

    auto repoStatus = OrnZypp::instance()->repoStatus(mRepoAlias);
    if (mRepoStatus != repoStatus)
    {
        qDebug() << mPackageName << "repo" << mRepoAlias << "status is" << repoStatus;
        mRepoStatus = repoStatus;
        emit this->repoStatusChanged();
        this->onAvailablePackagesChanged();
        this->onInstalledPackagesChanged();
    }
}

void OrnApplication::onAvailablePackagesChanged()
{
    if (mPackageName.isEmpty() || mRepoAlias.isEmpty())
    {
        return;
    }

    auto ornZypp = OrnZypp::instance();
    if (ornZypp->isAvailable(mPackageName))
    {
        auto ids = ornZypp->availablePackages(mPackageName);
        auto newest = mAvailableVersion;
        QString newestId;
        for (const auto &id : ids)
        {
            auto idParts = id.split(QChar(';'));
            auto repo = idParts.last();
            if (repo == mRepoAlias || repo == OrnZypp::installed)
            {
                // Install packages only from current repo
                auto version = idParts[1];
                if (OrnVersion(newest) < OrnVersion(version))
                {
                    newest = version;
                    newestId = id;
                }
            }
        }
        if (mAvailableVersion != newest)
        {
            mAvailableVersion = newest;
            mAvailablePackageId = newestId;
            emit this->availableVersionChanged();
        }
    }
    else if (!mAvailableVersion.isEmpty())
    {
        mAvailableVersion.clear();
        mAvailablePackageId.clear();
        emit this->availableVersionChanged();
        emit this->appNotFound();
    }
}

void OrnApplication::onInstalledPackagesChanged()
{
    if (mPackageName.isEmpty())
    {
        return;
    }

    auto ornZypp = OrnZypp::instance();
    if (ornZypp->isInstalled(mPackageName))
    {
        auto id = ornZypp->installedPackage(mPackageName);
        auto version = PackageKit::Transaction::packageVersion(id);
        if (mInstalledVersion != version)
        {
            mInstalledVersion = version;
            mInstalledPackageId = id;
            emit this->installedVersionChanged();
        }
    }
    else if (!mInstalledVersion.isEmpty())
    {
        mInstalledVersion.clear();
        mInstalledPackageId.clear();
        emit this->installedVersionChanged();
    }
    this->checkDesktopFile();
}

void OrnApplication::checkUpdates()
{
    auto updateAvailable = mInstalledVersion.isEmpty() ? false :
        OrnVersion(mInstalledVersion) < OrnVersion(mAvailableVersion);
    if (mUpdateAvailable != updateAvailable)
    {
        mUpdateAvailable = updateAvailable;
        emit this->updateAvailableChanged();
    }
}

void OrnApplication::onPackageInstalled(const QString &packageId)
{
    auto idParts = packageId.split(QChar(';'));
    if (idParts.first() == mPackageName)
    {
        mInstalledPackageId = packageId;
        mInstalledVersion = idParts[1];
        emit this->installedVersionChanged();
        this->checkDesktopFile();
        emit this->installed();
    }
}

void OrnApplication::onPackageRemoved(const QString &packageId)
{
    auto name = PackageKit::Transaction::packageName(packageId);
    if (name == mPackageName)
    {
        mInstalledPackageId.clear();
        mInstalledVersion.clear();
        emit this->installedVersionChanged();
        this->checkDesktopFile();
        emit this->removed();
    }
}

void OrnApplication::checkDesktopFile()
{
    auto desktopFile = mDesktopFile;
    if (mPackageName.isEmpty() || mInstalledPackageId.isEmpty())
    {
        desktopFile.clear();
    }
    else
    {
        auto desktopFiles = PackageKit::Transaction::packageDesktopFiles(mPackageName);
        for (const auto &file : desktopFiles)
        {
            if (QFileInfo(file).isFile())
            {
                desktopFile = file;
                break;
            }
        }
    }
    if (mDesktopFile != desktopFile)
    {
        qDebug() << "Using desktop file" << desktopFile;
        mDesktopFile = desktopFile;
        emit this->canBeLaunchedChanged();
    }
}
