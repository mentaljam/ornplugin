#include "orncommentsmodel.h"
#include "ornapirequest.h"
#include "orncommentlistitem.h"
#include "orn.h"

#include <QNetworkReply>
#include <QDebug>

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

void OrnCommentsModel::addComment(const quint32 &cid)
{
    auto reply = this->fetchComment(cid);
    connect(reply, &QNetworkReply::finished, [=]()
    {
        auto jsonObject = this->processReply(reply);
        if (jsonObject.isEmpty())
        {
            return;
        }
        this->beginInsertRows(QModelIndex(), 0, 0);
        mData.prepend(new OrnCommentListItem(jsonObject, this));
        this->endInsertRows();
    });
}

void OrnCommentsModel::editComment(const quint32 &cid)
{
    auto reply = this->fetchComment(cid);
    connect(reply, &QNetworkReply::finished, [=]()
    {
        auto jsonObject = this->processReply(reply);
        if (jsonObject.isEmpty())
        {
            return;
        }
        auto cid = Orn::toUint(jsonObject[QStringLiteral("cid")]);
        auto size = mData.size();
        for (int i = 0; i < size; ++i)
        {
            auto comment = static_cast<OrnCommentListItem *>(mData[i]);
            if (comment->mCid == cid)
            {
                comment->mText = Orn::toString(jsonObject[QStringLiteral("text")]);
                auto index = this->createIndex(i, 0);
                emit this->dataChanged(index, index);
                return;
            }
        }
        qWarning() << "Could not find comment in model with such id" << cid;
    });
}

QNetworkReply *OrnCommentsModel::fetchComment(const quint32 &cid)
{
    // FIXME: need refactoring
    auto manager = mApiRequest->networkManager();
    Q_ASSERT_X(manager, Q_FUNC_INFO, "networkManager must be set");
    auto url = OrnApiRequest::apiUrl(QStringLiteral("comments/%0").arg(cid));
    auto request = OrnApiRequest::networkRequest();
    request.setUrl(url);
    qDebug() << "Fetching data from" << request.url().toString();
    return manager->get(request);
}

QJsonObject OrnCommentsModel::processReply(QNetworkReply *reply)
{
    // FIXME: need refactoring
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Network request error" << reply->error()
                 << "-" << reply->errorString();
        reply->deleteLater();
        return QJsonObject();
    }

    QJsonParseError error;
    auto jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error == QJsonParseError::NoError)
    {
        return jsonDoc.object();
    }
    else
    {
        qCritical() << "Could not parse reply:" << error.errorString();
    }
    reply->deleteLater();
    return QJsonObject();
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

QHash<int, QByteArray> OrnCommentsModel::roleNames() const
{
    return { { Qt::DisplayRole, "commentData" } };
}

//bool OrnCommentsModel::removeRows(int row, int count, const QModelIndex &parent)
//{
//    if (parent.isValid())
//    {
//        return false;
//    }
//    auto last = row + count;
//    this->beginRemoveRows(parent, row, last - 1);
//    QObjectList list;
//    for (auto i = row; i < last; ++i)
//    {
//        list << mData[i];
//        mData.removeAt(i);
//    }
//    this->endRemoveRows();
//    qDeleteAll(list);
//    return true;
//}

void OrnCommentsModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    OrnAbstractListModel::processReply<OrnCommentListItem>(jsonDoc);
}

