#include "ornabstractlistmodel.h"
#include "ornapirequest.h"

#include <QNetworkReply>
#include <QDebug>

OrnAbstractListModel::OrnAbstractListModel(bool fetchable, QObject *parent) :
    QAbstractListModel(parent),
    mFetchable(fetchable),
    mCanFetchMore(true),
    mPage(0),
    mApiRequest(new OrnApiRequest(this))
{
    connect(mApiRequest, &OrnApiRequest::networkManagerChanged, this, &OrnAbstractListModel::reset);
    connect(mApiRequest, &OrnApiRequest::jsonReady, this, &OrnAbstractListModel::onJsonReady);
}

OrnApiRequest *OrnAbstractListModel::apiRequest() const
{
    return mApiRequest;
}

void OrnAbstractListModel::reset()
{
    qDebug() << "Resetting model";
    this->beginResetModel();
    auto d = mData;
    mData.clear();
    mCanFetchMore = true;
    mPage = 0;
    mApiRequest->reset();
    mPrevReplyHash.clear();
    this->endResetModel();
    // Delete data only after reset finished
    qDeleteAll(d);
}

void OrnAbstractListModel::apiCall(const QString &resource, QUrlQuery query)
{
    auto url = OrnApiRequest::apiUrl(resource);
    if (mFetchable)
    {
        query.addQueryItem(QStringLiteral("page"), QString::number(mPage));
    }
    if (!query.isEmpty())
    {
        url.setQuery(query);
    }
    auto request = OrnApiRequest::networkRequest();
    request.setUrl(url);
    mApiRequest->run(request);
}

int OrnAbstractListModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? mData.size() : 0;
}

bool OrnAbstractListModel::canFetchMore(const QModelIndex &parent) const
{
#ifndef NDEBUG
    if (!mApiRequest->networkManager())
    {
        qWarning() << "Network manager is not set";
    }
#endif
    return (!parent.isValid() && mApiRequest->networkManager()) ? mCanFetchMore : false;
}
