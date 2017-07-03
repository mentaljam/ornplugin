#include "orncommentsmodel.h"
#include "ornapirequest.h"
#include "orncommentlistitem.h"

OrnCommentsModel::OrnCommentsModel(QObject *parent) :
    OrnAbstractListModel(false, parent)
{

}

quint32 OrnCommentsModel::appId() const
{
    return mAppId;
}

void OrnCommentsModel::setAppId(const quint32 &appId)
{
    if (mAppId != appId)
    {
        mAppId = appId;
        emit this->appIdChanged();
        this->reset();
    }
}

QVariant OrnCommentsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    return QVariant::fromValue(mData[index.row()]);
}

void OrnCommentsModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
    {
        return;
    }
    OrnAbstractListModel::apiCall(QStringLiteral("apps/%0/comments").arg(mAppId));
}

bool OrnCommentsModel::canFetchMore(const QModelIndex &parent) const
{
    return (!parent.isValid() && mApiRequest->networkManager()) ? mCanFetchMore : false;
}

QHash<int, QByteArray> OrnCommentsModel::roleNames() const
{
    return { { Qt::DisplayRole, "commentData" } };
}

void OrnCommentsModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    OrnAbstractListModel::processReply<OrnCommentListItem>(jsonDoc);
}

