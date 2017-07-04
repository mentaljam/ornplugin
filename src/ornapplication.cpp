#include "ornapplication.h"
#include "orn.h"
#include "ornversion.h"

#include <PackageKit/packagekit-qt5/Daemon>

#include <QUrl>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

OrnApplication::OrnApplication(QObject *parent) :
    OrnApiRequest(parent),
    mUpdateAvailable(false),
    mRepoStatus(RepoNotInstalled),
    mAppId(0),
    mUserId(0),
    mRatingCount(0),
    mCommentsCount(0),
    mRating(0.0)
{
    connect(this, &OrnApplication::jsonReady, this, &OrnApplication::onJsonREady);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::repoListChanged,
            this, &OrnApplication::onRepoListChanged);
    connect(this, &OrnApplication::repoStatusChanged, this, &OrnApplication::onRepoStatusChanged);
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

void OrnApplication::enableRepo()
{
    Q_ASSERT_X(!mRepoId.isEmpty(), Q_FUNC_INFO, "Repo ID is empty. Run setAppId() and then update()");
    if (mRepoStatus == RepoNotInstalled)
    {
        if (Orn::addRepo(mUserName))
        {
            qDebug() << "Installed repo" << mRepoId;
            mRepoStatus = RepoEnabled;
            emit this->repoStatusChanged();
        }
        else
        {
            qWarning() << "Could not install repo" << mRepoId;
            emit this->errorInstallRepo();
        }
    }
    else if (mRepoStatus == RepoDisabled)
    {
        auto t = this->transaction();
        connect(t, &PackageKit::Transaction::finished, [=]()
        {
            qDebug() << "Starting transaction" << t->uid() << "method checkRepoUpdate()";
            this->checkRepoUpdate(mRepoId, QString(), true);
        });
        qDebug() << "Starting transaction" << t->uid() << "method repoEnable()";
        t->repoEnable(mRepoId, true);
    }
    else
    {
        qDebug() << "Repository is already enabled";
    }
}

void OrnApplication::install()
{
    auto t = this->transaction();
    connect(t, &PackageKit::Transaction::finished, [=]()
    {
        this->onInstalled(mAvailablePackageId, mAvailableVersion);
    });
    qDebug() << "Installing" << mPackageName << "by starting transaction"
             << t->uid() << "method installPackage()";
    t->installPackage(mAvailablePackageId);
}

void OrnApplication::remove()
{
    auto t = this->transaction();
    connect(t, &PackageKit::Transaction::package, [=]()
    {
        qDebug() << "Package" << mInstalledPackageId << "was removed";
        mInstalledPackageId.clear();
        mInstalledVersion.clear();
        emit this->installedVersionChanged();
    });
    qDebug() << "Removing" << mPackageName << "by starting transaction"
             << t->uid() << "method removePackage()";
    t->removePackage(mInstalledPackageId);
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

PackageKit::Transaction *OrnApplication::transaction()
{
    auto t = new PackageKit::Transaction();
    connect(t, &PackageKit::Transaction::finished, this, &OrnApplication::onTransactionFinished);
#ifndef NDEBUG
    connect(t, &PackageKit::Transaction::errorCode,
            [=](PackageKit::Transaction::Error error, const QString &details)
    {
        qDebug() << "An error occured while running transaction" << t->uid()
                 << ":" << error << "-" << details;
    });
#endif
    return t;
}

void OrnApplication::onRepoListChanged()
{
    auto t = this->transaction();
    connect(t, &PackageKit::Transaction::repoDetail, this, &OrnApplication::checkRepoUpdate);
    qDebug() << "Repositories list has been changed. Starting transaction" << t->uid()
             << "method getRepoList() to check for application"
             << mPackageName << "repository" << mRepoId << "changes.";
    t->getRepoList();
}

void OrnApplication::checkAppPackage(const PackageKit::Transaction::Filter &filter)
{
    auto t = this->transaction();
    connect(t, &PackageKit::Transaction::package, this, &OrnApplication::onPackage);
    qDebug() << "Resolving package" << mPackageName
             << PackageKit::Daemon::enumToString<PackageKit::Transaction>(filter, "Filter")
             << "packages by starting transaction" << t->uid() << "method resolve()";
    t->resolve(mPackageName, filter);
}

void OrnApplication::onRepoStatusChanged()
{
    if (mRepoStatus != RepoEnabled)
    {
        return;
    }
    auto t = this->transaction();
    connect(t, &PackageKit::Transaction::finished, [=](PackageKit::Transaction::Exit status, uint runtime)
    {
        // Get available package versions after repository refresh
        Q_UNUSED(runtime)
        if (status == PackageKit::Transaction::ExitSuccess)
        {
            this->checkAppPackage(PackageKit::Transaction::FilterNotInstalled);
        }
    });
    qDebug() << "Refreshing" << mRepoId << "by starting transaction"
             << t->uid() << "method repoSetData()";
    t->repoSetData(mRepoId, QStringLiteral("refresh-now"), QStringLiteral("false"));
}

void OrnApplication::onTransactionFinished(PackageKit::Transaction::Exit status, uint runtime)
{
    auto t = static_cast<PackageKit::Transaction *>(QObject::sender());
    qDebug() << "Transaction" << t->uid() << "finished in" << runtime << "msec" << "with status" << status;
    t->deleteLater();
}

void OrnApplication::checkRepoUpdate(const QString &repoId, const QString &description, bool enabled)
{
    Q_UNUSED(description)
    if (mRepoId != repoId)
    {
        return;
    }
    // NOTE: will this work for removed repos?
    auto status = Orn::isRepoInstalled(repoId) ? RepoDisabled : RepoNotInstalled;
    if (enabled)
    {
        status = RepoEnabled;
    }
    if (mRepoStatus != status)
    {
        mRepoStatus = status;
        qDebug() << "Application" << mPackageName << "repository status:" << status;
        emit this->repoStatusChanged();
    }
}

void OrnApplication::onPackage(PackageKit::Transaction::Info info, const QString &packageID, const QString &summary)
{
    Q_UNUSED(summary)
    auto idParts = packageID.split(QChar(';'));
    if (idParts[0] != mPackageName)
    {
        return;
    }

    auto version = idParts[1];

    switch (info)
    {
    case PackageKit::Transaction::InfoInstalled:
        this->onInstalled(packageID, version);
        break;
    case PackageKit::Transaction::InfoAvailable:
        if (OrnVersion(mAvailableVersion) < OrnVersion(version))
        {
            mAvailablePackageId = packageID;
            mAvailableVersion = version;
            emit this->availableVersionChanged();
        }
        break;
    default:
        break;
    }
}

void OrnApplication::onInstalled(const QString &packageId, const QString &version)
{
    if (mInstalledPackageId == packageId)
    {
        qWarning() << "It looks like a package with such id is already installed" << packageId;
        return;
    }
    qDebug() << "Package" << packageId << "is installed";
    mInstalledPackageId = packageId;
    mInstalledVersion = version;
    emit this->installedVersionChanged();
    auto desktopFiles = PackageKit::Transaction::packageDesktopFiles(mPackageName);
    if (!desktopFiles.isEmpty() && mDesktopFile != desktopFiles[0])
    {
        mDesktopFile = desktopFiles[0];
        emit this->canBeLaunchedChanged();
    }
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

void OrnApplication::onJsonREady(const QJsonDocument &jsonDoc)
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

    mTagsIds = Orn::toIntList(QStringLiteral("tags"));
    mCategoryIds = Orn::toIntList(QStringLiteral("category"));

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
        mRepoId = Orn::repoNamePrefix + mUserName;
        // Check if repository is enabled
        this->onRepoListChanged();
        // Check if application is installed
        this->checkAppPackage(PackageKit::Transaction::FilterInstalled);
    }
    else
    {
        mRepoId.clear();
    }

    qDebug() << "Application" << mPackageName << "information updated";
    emit this->updated();
}
