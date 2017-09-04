#include "ornbackup.h"
#include "ornzypp.h"
#include "ornversion.h"
#include "ornclient.h"

#include <QFileInfo>
#include <QSettings>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

#include <QDebug>

#define BR_CREATED       QStringLiteral("created")
#define BR_REPO_ALL      QStringLiteral("repos/all")
#define BR_REPO_DISABLED QStringLiteral("repos/disabled")
#define BR_INSTALLED     QStringLiteral("packages/installed")
#define BR_BOOKMARKS     QStringLiteral("packages/bookmarks")

OrnBackup::OrnBackup(QObject *parent) :
    QObject(parent),
    mZypp(OrnZypp::instance()),
    mStatus(Idle)
{

}

OrnBackup::Status OrnBackup::status() const
{
    return mStatus;
}

void OrnBackup::setStatus(const Status &status)
{
    if (mStatus != status)
    {
        mStatus = status;
        emit this->statusChanged();
    }
}

QVariantMap OrnBackup::details(const QString &path)
{
    Q_ASSERT_X(QFileInfo(path).isFile(), Q_FUNC_INFO, "Backup file does not exist");

    QVariantMap res;
    QSettings file(path, QSettings::IniFormat);

    res.insert(QLatin1String("created"),   file.value(BR_CREATED).toDateTime().toLocalTime());
    res.insert(QLatin1String("repos"),     file.value(BR_REPO_ALL).toStringList().size());
    res.insert(QLatin1String("packages"),  file.value(BR_INSTALLED).toStringList().size());
    res.insert(QLatin1String("bookmarks"), file.value(BR_BOOKMARKS).toStringList().size());

    return res;
}

void OrnBackup::backup(const QString &filePath)
{
    Q_ASSERT_X(!filePath.isEmpty(), Q_FUNC_INFO, "A file path must be provided");
    Q_ASSERT_X(!QFileInfo(filePath).isFile(), Q_FUNC_INFO, "Backup file already exists");

    if (mStatus != Idle)
    {
        qWarning() << this << "is already" << mStatus;
        return;
    }

    mFilePath = filePath;
    auto dir = QFileInfo(mFilePath).dir();
    if (!dir.exists() && !dir.mkpath(QChar('.')))
    {
        qCritical() << "Failed to create directory" << dir.absolutePath();
        emit this->backupError(DirectoryError);
    }
    qDebug() << mFilePath;
    QtConcurrent::run(this, &OrnBackup::pBackup);
}

void OrnBackup::restore(const QString &filePath)
{
    Q_ASSERT_X(!filePath.isEmpty(), Q_FUNC_INFO, "A file path must be set");
    Q_ASSERT_X(QFileInfo(filePath).isFile(), Q_FUNC_INFO, "Backup file does not exist");

    if (mStatus != Idle)
    {
        qWarning() << this << "is already" << mStatus;
        return;
    }

    mFilePath = filePath;
    auto watcher = new QFutureWatcher<void>();
    connect(watcher, &QFutureWatcher<void>::finished, this, &OrnBackup::pRefreshRepos);
    watcher->setFuture(QtConcurrent::run(this, &OrnBackup::pRestore));
}

QStringList OrnBackup::notFound() const
{
    QStringList names;
    for (const auto &name : mPackagesToInstall.keys())
    {
        if (!mNamesToSearch.contains(name))
        {
            names << name;
        }
    }
    return names;
}

bool OrnBackup::removeFile(const QString &filePath)
{
    Q_ASSERT_X(!QFileInfo(filePath).isDir(), Q_FUNC_INFO, "Path must be a file");
    return QFile(filePath).remove();
}

void OrnBackup::pSearchPackages()
{
    qDebug() << "Searching packages";
    this->setStatus(SearchingPackages);

    // Delete future watcher and prepare variables
    this->sender()->deleteLater();
    mPackagesToInstall.clear();

    auto t = mZypp->transaction();
    connect(t, &PackageKit::Transaction::package, this, &OrnBackup::pAddPackage);
    connect(t, &PackageKit::Transaction::finished, this, &OrnBackup::pInstallPackages);
    t->searchNames(mNamesToSearch, PackageKit::Transaction::FilterNotInstalled);
}

void OrnBackup::pAddPackage(int info, const QString &packageId, const QString &summary)
{
    Q_UNUSED(info)
    Q_UNUSED(summary)
    auto idParts = packageId.split(QChar(';'));
    // Process only packages from OpenRepos
    if (idParts.last().startsWith(OrnZypp::repoNamePrefix))
    {
        // We will filter the newest versions later
        mPackagesToInstall.insertMulti(idParts.first(), packageId);
    }
}

void OrnBackup::pInstallPackages()
{
    qDebug() << "Installing packages";
    this->setStatus(InstallingPackages);

    QStringList ids;
    for (const auto &pname : mPackagesToInstall.uniqueKeys())
    {
        const auto &pids = mPackagesToInstall.values(pname);
        QString newestId;
        OrnVersion newestVersion;
        for (const auto &pid : pids)
        {
            OrnVersion v(PackageKit::Transaction::packageVersion(pid));
            if (v > newestVersion)
            {
                newestVersion = v;
                newestId = pid;
            }
        }
        ids << newestId;
    }

    if (ids.isEmpty())
    {
        this->pFinishRestore();
    }
    else
    {
        auto t = mZypp->transaction();
        connect(t, &PackageKit::Transaction::finished, this, &OrnBackup::pFinishRestore);
        t->installPackages(ids);
    }
}

void OrnBackup::pFinishRestore()
{
    qDebug() << "Finished restoring";
    mFilePath.clear();
    this->setStatus(Idle);
    emit this->restored();
}

void OrnBackup::pBackup()
{
    qDebug() << "Starting backing up";
    this->setStatus(BackingUp);
    QSettings file(mFilePath, QSettings::IniFormat);
    QStringList repos;
    QStringList disabled;
    QSet<QString> ornPackages;

    for (auto it = mZypp->mRepos.cbegin(); it != mZypp->mRepos.cend(); ++it)
    {
        auto author = it.key().mid(OrnZypp::repoNamePrefixLength);
        repos << author;
        auto repo = it.value();
        if (!repo.enabled)
        {
            disabled << author;
        }
        for (const auto &package : repo.packages)
        {
            ornPackages.insert(package);
        }
    }

    qDebug() << "Backing up repos";
    file.setValue(BR_REPO_ALL, repos);
    file.setValue(BR_REPO_DISABLED, disabled);

    qDebug() << "Backing up installed packages";
    QStringList installed;
    for (const auto &name :  mZypp->mInstalledPackages.uniqueKeys())
    {
        if (ornPackages.contains(name))
        {
            installed << name;
        }
    }
    file.setValue(BR_INSTALLED, installed);

    qDebug() << "Backing up bookmarks";
    QVariantList bookmarks;
    for (const auto &b : OrnClient::instance()->mBookmarks)
    {
        bookmarks << b;
    }
    file.setValue(BR_BOOKMARKS, bookmarks);

    file.setValue(BR_CREATED, QDateTime::currentDateTime().toUTC());
    qDebug() << "Finished backing up";
    mFilePath.clear();
    this->setStatus(Idle);
    emit this->backedUp();
}

void OrnBackup::pRestore()
{
    QSettings file(mFilePath, QSettings::IniFormat);

    qDebug() << "Restoring bookmarks";
    this->setStatus(RestoringBookmarks);
    auto client = OrnClient::instance();
    auto oldBookmarks = client->mBookmarks;
    for (const auto &b : file.value(BR_BOOKMARKS).toList())
    {
        client->mBookmarks.insert(b.toUInt());
    }
    if (oldBookmarks != client->mBookmarks)
    {
        emit client->bookmarksChanged();
    }

    qDebug() << "Restoring repos";
    this->setStatus(RestoringRepos);

    auto repos = file.value(BR_REPO_ALL).toStringList();
    auto disabled = file.value(BR_REPO_DISABLED).toStringList().toSet();
    mNamesToSearch = file.value(BR_INSTALLED).toStringList();

    for (const auto &author : repos)
    {
        auto alias = OrnZypp::repoNamePrefix + author;
        auto call = QDBusMessage::createMethodCall(OrnZypp::ssuInterface, OrnZypp::ssuPath,
                                                   OrnZypp::ssuInterface, OrnZypp::ssuAddRepo);
        call.setArguments(QVariantList{ alias, OrnZypp::repoBaseUrl.arg(author) });
        QDBusConnection::systemBus().call(call, QDBus::Block);
        mZypp->mRepos.insert(alias, OrnZypp::RepoMeta(!disabled.contains(author)));
    }
}

void OrnBackup::pRefreshRepos()
{
    qDebug() << "Refreshing repos";
    this->setStatus(RefreshingRepos);
    auto t = new PackageKit::Transaction();
    connect(t, &PackageKit::Transaction::finished, t, &PackageKit::Transaction::deleteLater);
    connect(t, &PackageKit::Transaction::finished, this, &OrnBackup::pSearchPackages);
    t->refreshCache(false);
}
