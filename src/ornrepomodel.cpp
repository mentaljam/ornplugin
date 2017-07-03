#include "ornrepomodel.h"
#include "orn.h"

#include <PackageKit/packagekit-qt5/Transaction>

#include <QFile>
#include <QDebug>

OrnRepoModel::OrnRepoModel(QObject *parent) :
    QAbstractListModel(parent)
{
    this->reset();
}

void OrnRepoModel::reset()
{
    qDebug() << "Clearing model";
    this->beginResetModel();
    mData.clear();
    this->endResetModel();
    auto t = this->transaction();
    qDebug() << "Starting transaction" << t->uid() << "method getRepoList()";
    t->getRepoList();
}

void OrnRepoModel::enableRepo(const QString &repoId, bool enable)
{
    auto t = this->transaction();
    qDebug() << "Starting transaction" << t->uid() << "method repoEnable()";
    t->repoEnable(repoId, enable);
    // Transaction::repoEnable() does not emmit any signal?
    this->onRepoUpdated(repoId, QString(), enable);
}

void OrnRepoModel::refreshRepo(const QString &repoId)
{
    auto t = this->transaction();
    qDebug() << "Starting transaction" << t->uid() << "method repoSetData()";
    t->repoSetData(repoId, QStringLiteral("refresh-now"), QStringLiteral("false"));
}

void OrnRepoModel::removeRepo(const QString &repoAuthor)
{
    auto size = mData.size();
    for (int i = 0; i < size; ++i)
    {
        if (mData[i].author == repoAuthor)
        {
            if (Orn::modifyRepo(repoAuthor, Orn::RemoveRepo))
            {
                this->beginRemoveRows(QModelIndex(), i, i);
                mData.removeAt(i);
                this->endRemoveRows();
            }
            else
            {
                qWarning() << "Could not remove repo"
                           << Orn::repoNamePrefix + repoAuthor;
                emit this->errorRemoveRepo();
            }
            return;
        }
    }
    qWarning() << "Could not find repo" << Orn::repoNamePrefix + repoAuthor
               << "to remove";
    emit this->errorRemoveRepo();
}

void OrnRepoModel::onRepoUpdated(const QString &repoId, const QString &description, bool enabled)
{
    Q_UNUSED(description)

    if (!repoId.startsWith(Orn::repoNamePrefix))
    {
        qDebug() << repoId << "is not an OpenRepos repo";
        return;
    }

    // Remove "openrepos-" prefix
    auto author = repoId.mid(Orn::repoNamePrefix.length());
    int row = 0;
    for (auto &repo: mData)
    {
        if (repo.author == author)
        {
            if (repo.enabled != enabled)
            {
                repo.enabled = enabled;
                qDebug() << "Updating repo" << repo.id;
                auto index = this->createIndex(row, 0);
                emit this->dataChanged(index, index, { RepoEnabledRole });
            }
            // Nothing to be done
            return;
        }
        ++row;
    }

    qDebug() << "Appending new repo" << repoId;
    row = mData.size();
    this->beginInsertRows(QModelIndex(), row, row);
    mData << Repo{ enabled, repoId, author };
    this->endInsertRows();
}

void OrnRepoModel::onFinished(int status, uint runtime)
{
    auto t = static_cast<PackageKit::Transaction *>(QObject::sender());
    qDebug() << "Transaction" << t->uid() << "finished in"
             << runtime << "msec" << "with status" << status;
    t->deleteLater();
}

PackageKit::Transaction *OrnRepoModel::transaction() const
{
    auto t = new PackageKit::Transaction();
    connect(t, &PackageKit::Transaction::finished, this, &OrnRepoModel::onFinished);
    connect(t, &PackageKit::Transaction::repoDetail, this, &OrnRepoModel::onRepoUpdated);
    return t;
}

int OrnRepoModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? mData.size() : 0;
}

QVariant OrnRepoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    auto &repo = mData[index.row()];
    switch (role)
    {
    case RepoAuthorRole:
        return repo.author;
    case RepoIdRole:
        return repo.id;
    case RepoEnabledRole:
        return repo.enabled;
    case SortRole:
        // Place enabled first and then sort by author
        return QString::number(!repo.enabled).append(repo.author);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OrnRepoModel::roleNames() const
{
    return {
        { RepoAuthorRole,  "repoAuthor"  },
        { RepoIdRole,      "repoId"      },
        { RepoEnabledRole, "repoEnabled" }
    };
}
