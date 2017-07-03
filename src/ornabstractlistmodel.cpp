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
    qDeleteAll(mData);
    mData.clear();
    mCanFetchMore = true;
    mPage = 0;
    mApiRequest->reset();
    this->endResetModel();
}

void OrnAbstractListModel::apiCall(const QString &resource)
{
    auto url = OrnApiRequest::apiUrl(resource);
    if (mFetchable)
    {
        url.setQuery(QStringLiteral("page=").append(QString::number(mPage)));
    }
    auto request = OrnApiRequest::networkRequest();
    request.setUrl(url);
    mApiRequest->run(request);
}

int OrnAbstractListModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? mData.size() : 0;
}
