#include "ornabstractappsmodel.h"
#include "ornapplistitem.h"
#include "ornapirequest.h"
#include "ornpm.h"

OrnAbstractAppsModel::OrnAbstractAppsModel(bool fetchable, QObject *parent)
    : OrnAbstractListModel(fetchable, parent)
{
    connect(OrnPm::instance(), &OrnPm::packageStatusChanged,
            [this](const QString &packageName, const OrnPm::PackageStatus &status)
    {
        Q_UNUSED(status)

        auto size = mData.size();
        for (int i = 0; i < size; ++i)
        {
            auto app = static_cast<OrnAppListItem *>(mData[i]);
            if (app->package == packageName)
            {
                auto ind = this->createIndex(i, 0);
                emit this->dataChanged(ind, ind, {PackageStatusRole});
                return;
            }
        }
    });
}

QVariant OrnAbstractAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    auto app = static_cast<OrnAppListItem *>(mData[index.row()]);
    switch (role)
    {
    case SortRole:
        return app->title.toLower();
    case PackageStatusRole:
        return OrnPm::instance()->packageStatus(app->package);
    case AppIdRole:
        return app->appId;
    case CreateDateRole:
        return QDateTime::fromMSecsSinceEpoch(quint64(app->created) * 1000).date();
    case RatingCountRole:
        return app->ratingCount;
    case RatingRole:
        return app->rating;
    case TitleRole:
        return app->title;
    case UserNameRole:
        return app->userName;
    case IconSourceRole:
        return app->iconSource;
    case SinceUpdateRole:
        return app->sinceUpdate;
    case CategoryRole:
        return app->category;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OrnAbstractAppsModel::roleNames() const
{
    return {
        { PackageStatusRole, "packageStatus" },
        { AppIdRole,         "appId" },
        { CreateDateRole,    "createDate" },
        { RatingCountRole,   "ratingCount" },
        { RatingRole,        "rating" },
        { TitleRole,         "title" },
        { UserNameRole,      "userName" },
        { IconSourceRole,    "iconSource" },
        { SinceUpdateRole,   "sinceUpdate" },
        { CategoryRole,      "category" }
    };
}

void OrnAbstractAppsModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    OrnAbstractListModel::processReply<OrnAppListItem>(jsonDoc);
}
