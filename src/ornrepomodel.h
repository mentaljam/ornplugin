#ifndef ORNREPOMODEL_H
#define ORNREPOMODEL_H

#include <QAbstractListModel>

namespace PackageKit {
class Transaction;
}

class OrnRepoModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasEnabledRepos READ hasEnabledRepos NOTIFY enabledReposChanged)
    Q_PROPERTY(bool hasDisabledRepos READ hasDisabledRepos NOTIFY enabledReposChanged)

public:

    enum Roles
    {
        RepoAuthorRole = Qt::UserRole + 1,
        RepoIdRole,
        RepoEnabledRole,
        SortRole
    };
    Q_ENUM(Roles)

    explicit OrnRepoModel(QObject *parent = 0);

    bool hasEnabledRepos() const;
    bool hasDisabledRepos() const;

public slots:
    void reset();
    void enableRepo(const QString &repoId, bool enable);
    void enableRepos(bool enable);
    void refreshRepo(const QString &repoId);
    void removeRepo(const QString &repoAuthor);

signals:
    void errorRemoveRepo();
    void enabledReposChanged();

private slots:
    void onRepoUpdated(const QString &repoId, const QString &description, bool enabled);
    void onFinished(int status, uint runtime);

private:
    PackageKit::Transaction *transaction() const;

private:

    struct Repo
    {
        bool enabled;
        QString id;
        QString author;
    };

    int mEnabledRepos;
    QList<Repo> mData;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};

#endif // ORNREPOMODEL_H
