#include "ornrepomodel.h"
#include "orn.h"
#include "ornzypp.h"

#include <PackageKit/packagekit-qt5/Transaction>

#include <QFile>
#include <QDebug>

OrnRepoModel::OrnRepoModel(QObject *parent) :
    QAbstractListModel(parent),
    mEnabledRepos(0)
{
    this->reset();
}

bool OrnRepoModel::hasEnabledRepos() const
{
    return mEnabledRepos;
}

bool OrnRepoModel::hasDisabledRepos() const
{
    return mEnabledRepos != mData.size();
}

void OrnRepoModel::reset()
{
    qDebug() << "Resetting model";
    this->beginResetModel();
    mData.clear();
    this->endResetModel();
    if (mEnabledRepos != 0)
    {
        mEnabledRepos = 0;
        emit this->enabledReposChanged();
    }
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

void OrnRepoModel::enableRepos(bool enable)
{
    for (const auto &repo : mData)
    {
        this->enableRepo(repo.id, enable);
    }
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
            if (OrnZypp::repoAction(repoAuthor, OrnZypp::RemoveRepo))
            {
                this->beginRemoveRows(QModelIndex(), i, i);
                mData.removeAt(i);
                this->endRemoveRows();
                --mEnabledRepos;
                emit this->enabledReposChanged();
            }
            else
            {
                qWarning() << "Could not remove repo"
                           << OrnZypp::repoNamePrefix + repoAuthor;
                emit this->errorRemoveRepo();
            }
            return;
        }
    }
    qWarning() << "Could not find repo" << OrnZypp::repoNamePrefix + repoAuthor
               << "to remove";
    emit this->errorRemoveRepo();
}

void OrnRepoModel::onRepoUpdated(const QString &repoId, const QString &description, bool enabled)
{
    Q_UNUSED(description)

    if (!repoId.startsWith(OrnZypp::repoNamePrefix))
    {
        qDebug() << repoId << "is not an OpenRepos repo";
        return;
    }

    // Remove "openrepos-" prefix
    auto author = repoId.mid(OrnZypp::repoNamePrefixLength);
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
                mEnabledRepos += enabled ? 1 : -1;
                emit this->enabledReposChanged();
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
    mEnabledRepos += enabled;
    emit this->enabledReposChanged();
}

PackageKit::Transaction *OrnRepoModel::transaction() const
{
    auto t = Orn::transaction();
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
