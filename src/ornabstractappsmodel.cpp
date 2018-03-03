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
        auto size = mData.size();
        for (int i = 0; i < size; ++i)
        {
            auto app = static_cast<OrnAppListItem *>(mData[i]);
            if (app->mPackage == packageName)
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
    case DataRole:
        return QVariant::fromValue(app);
    case TitleRole:
        return app->mTitle.toLower();
    case DateRole:
        return app->mCreated;
    case PackageStatusRole:
        return OrnPm::instance()->packageStatus(app->mPackage);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OrnAbstractAppsModel::roleNames() const
{
    return {
        { DataRole, "appData" },
        { PackageStatusRole, "packageStatus" }
    };
}

void OrnAbstractAppsModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    OrnAbstractListModel::processReply<OrnAppListItem>(jsonDoc);
}
