#include "ornrepomodel.h"

#include <QTimer>
#include <QDebug>

OrnRepoModel::OrnRepoModel(QObject *parent) :
    QAbstractListModel(parent),
    mEnabledRepos(0)
{
    auto ornZypp = OrnZypp::instance();

    connect(ornZypp, &OrnZypp::endRepoFetching, this, &OrnRepoModel::reset);
    connect(ornZypp, &OrnZypp::repoModified, this, &OrnRepoModel::onRepoModified);
    connect(this, &OrnRepoModel::modelReset, this, &OrnRepoModel::enabledReposChanged);

    // Delay reset to ensure that modelReset signal is received in qml
    QTimer::singleShot(500, this, &OrnRepoModel::reset);
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

    auto repos = OrnZypp::instance()->repoList();
    auto size = repos.size();
    int enabledRepos = 0;
    if (size)
    {
        mData.append(repos);
        for (const auto &repo : repos)
        {
            enabledRepos += repo.enabled;
        }
    }

    if (mEnabledRepos != enabledRepos)
    {
        mEnabledRepos = enabledRepos;
        emit this->enabledReposChanged();
    }

    this->endResetModel();
}

void OrnRepoModel::onRepoModified(const QString &alias, const OrnZypp::RepoAction &action)
{
    if (action == OrnZypp::AddRepo)
    {
        auto row = mData.size();
        this->beginInsertRows(QModelIndex(), row, row);
        mData << OrnZypp::Repo{ true, alias, alias.mid(OrnZypp::repoNamePrefixLength) };
        ++mEnabledRepos;
        emit this->enabledReposChanged();
        this->endInsertRows();
        return;
    }

    int row = 0;
    for (auto &repo : mData)
    {
        if (repo.alias == alias)
        {
            switch (action)
            {
            case OrnZypp::RemoveRepo:
                this->beginRemoveRows(QModelIndex(), row, row);
                mData.removeAt(row);
                --mEnabledRepos;
                emit this->enabledReposChanged();
                this->endRemoveRows();
                break;
            case OrnZypp::DisableRepo:
            case OrnZypp::EnableRepo:
                {
                    bool enable = action == OrnZypp::EnableRepo;
                    if (repo.enabled != enable)
                    {
                        repo.enabled = enable;
                        auto index = this->createIndex(row, 0);
                        emit this->dataChanged(index, index, { RepoEnabledRole });
                        mEnabledRepos += enable ? 1 : -1;
                        emit this->enabledReposChanged();
                    }
                }
                break;
            default:
                break;
            }
            return;
        }
        ++row;
    }
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
    case RepoAliasRole:
        return repo.alias;
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
        { RepoAliasRole,   "repoAlias"   },
        { RepoEnabledRole, "repoEnabled" }
    };
}
