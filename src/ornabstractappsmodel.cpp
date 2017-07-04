#include "ornabstractappsmodel.h"
#include "ornapplistitem.h"
#include "ornapirequest.h"

OrnAbstractAppsModel::OrnAbstractAppsModel(bool fetchable, QObject *parent) :
    OrnAbstractListModel(fetchable, parent)
{

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
        return app->mUpdated;
    default:
        return QVariant();
    }
}

bool OrnAbstractAppsModel::canFetchMore(const QModelIndex &parent) const
{
#ifndef NDEBUG
    if (!mApiRequest->networkManager())
    {
        qWarning() << "Network manager is not set";
    }
#endif
    return (!parent.isValid() && mApiRequest->networkManager()) ? mCanFetchMore : false;
}

QHash<int, QByteArray> OrnAbstractAppsModel::roleNames() const
{
    return { { DataRole, "appData" } };
}

void OrnAbstractAppsModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    OrnAbstractListModel::processReply<OrnAppListItem>(jsonDoc);
}
