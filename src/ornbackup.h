#ifndef ORNBACKUP_H
#define ORNBACKUP_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QSet>

class OrnZypp;

class OrnBackup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:

    enum Status
    {
        Idle,
        BackingUp,
        RestoringBookmarks,
        RestoringRepos,
        RefreshingRepos,
        SearchingPackages,
        InstallingPackages
    };
    Q_ENUM(Status)

    enum Error
    {
        NoError,
        DirectoryError
    };
    Q_ENUMS(Error)

    explicit OrnBackup(QObject *parent = 0);

    Status status() const;

    Q_INVOKABLE static QVariantMap details(const QString &path);
    Q_INVOKABLE void backup(const QString &filePath);
    Q_INVOKABLE void restore(const QString &filePath);
    Q_INVOKABLE QStringList notFound() const;
    Q_INVOKABLE static bool removeFile(const QString &filePath);

signals:
    void statusChanged();
    void backupError(Error err);
    void backedUp();
    void restored();

private slots:
    void pSearchPackages();
    void pAddPackage(int info, const QString &packageId, const QString &summary);
    void pInstallPackages();
    void pFinishRestore();

private:
    void setStatus(const Status &status);
    void pBackup();
    void pRestore();
    void pRefreshRepos();

private:
    OrnZypp *mZypp;
    Status mStatus;
    QString mFilePath;
    QStringList mNamesToSearch;
    QMultiHash<QString, QString> mPackagesToInstall;
};

#endif // ORNBACKUP_H
