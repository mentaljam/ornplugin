#include "orninstalledappsmodel.h"
#include "orn.h"

#include <QDebug>

OrnInstalledAppsModel::OrnInstalledAppsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    auto ornZypp = OrnZypp::instance();
    connect(ornZypp, &OrnZypp::installedAppsReady,
            this, &OrnInstalledAppsModel::onInstalledAppsReady);
    connect(ornZypp, &OrnZypp::packageRemoved,
            this, &OrnInstalledAppsModel::onPackageRemoved);
    this->reset();
}

void OrnInstalledAppsModel::reset()
{
    qDebug() << "Resetting model";
    this->beginResetModel();
    OrnZypp::instance()->getInstalledApps();
}

void OrnInstalledAppsModel::onInstalledAppsReady(const OrnZypp::AppList &apps)
{
    if (!apps.empty())
    {
        mData.clear();
        mData.append(apps);
    }
    else
    {
        qWarning() << "App list is empty";
    }
    this->endResetModel();
}

void OrnInstalledAppsModel::onPackageRemoved(const QString &packageId)
{
    auto name = packageId.section(QChar(';'), 0, 0);
    auto size = mData.size();
    for (OrnZypp::AppList::size_type i = 0; i < size; ++i)
    {
        if (mData[i].name == name)
        {
            this->beginRemoveRows(QModelIndex(), i, i);
            mData.removeAt(i);
            this->endInsertRows();
            return;
        }
    }
}

int OrnInstalledAppsModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? mData.size() : 0;
}

QVariant OrnInstalledAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    auto app = mData[index.row()];
    switch (role)
    {
    case NameRole:
        return app.name;
    case TitleRole:
        return app.title;
    case VersionRole:
        return app.version;
    case AuthorRole:
        return app.author;
    case IconRole:
        return app.icon;
    case SortRole:
        // First show apps with available updates then sort by title
        return QString::number(app.updateId.isEmpty()).append(app.title);
    case SectionRole:
        return app.title.at(0).toUpper();
    case UpdateAvailableRole:
        // Return int to make it easier to parse
        return (int)!app.updateId.isEmpty();
    case PackageIdRole:
        return app.packageId;
    case UpdateIdRole:
        return app.updateId;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OrnInstalledAppsModel::roleNames() const
{
    return {
        { NameRole,    "appName"       },
        { TitleRole,   "appTitle"      },
        { VersionRole, "appVersion"    },
        { AuthorRole,  "appAuthor"     },
        { IconRole,    "appIconSource" },
        { SectionRole, "section"       },
        { UpdateAvailableRole, "updateAvailable" },
        { PackageIdRole, "packageId"     },
        { UpdateIdRole,  "updateId"      },
    };
}
