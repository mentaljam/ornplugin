#include "orncommentsmodel.h"
#include "ornapirequest.h"
#include "orncommentlistitem.h"

OrnCommentsModel::OrnCommentsModel(QObject *parent) :
    OrnAbstractListModel(false, parent)
{
    connect(this, &OrnCommentsModel::rowsInserted, [=](const QModelIndex &parent, int first, int last)
    {
        Q_UNUSED(parent)
        for (auto i = first; i < last; ++i)
        {
            auto comment = static_cast<OrnCommentListItem *>(mData[i]);
            mCommentsMap.insert(comment->mCid, comment);
        }
    });
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

OrnCommentListItem *OrnCommentsModel::findItem(const quint32 &cid) const
{
    return mCommentsMap.contains(cid) ? mCommentsMap[cid] : 0;
}

int OrnCommentsModel::findItemRow(const quint32 &cid) const
{
    QObjectList::size_type i = 0;
    for (const auto &c: mData)
    {
        if (static_cast<OrnCommentListItem *>(c)->mCid == cid)
        {
            return i;
        }
        ++i;
    }
    return -1;
}
QVariant OrnCommentsModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
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
#ifndef NDEBUG
    if (!mApiRequest->networkManager())
    {
        qWarning() << "Network manager is not set";
    }
#endif
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

