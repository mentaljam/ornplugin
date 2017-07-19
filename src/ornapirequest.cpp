#include "ornapirequest.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>

const QString OrnApiRequest::apiUrlPrefix(QStringLiteral("https://openrepos.net/api/v1/"));
const QByteArray OrnApiRequest::langName(QByteArrayLiteral("Accept-Language"));
const QByteArray OrnApiRequest::langValue(QLocale::system().name().left(2).toUtf8());
const QByteArray OrnApiRequest::platformName(QByteArrayLiteral("Warehouse-Platform"));
const QByteArray OrnApiRequest::platformValue(QByteArrayLiteral("SailfishOS"));

OrnApiRequest::OrnApiRequest(QObject *parent) :
    QObject(parent),
    mNetworkManager(0),
    mNetworkReply(0)
{

}

OrnApiRequest::~OrnApiRequest()
{
    if (mNetworkReply)
    {
        mNetworkReply->deleteLater();
    }
}

QNetworkAccessManager *OrnApiRequest::networkManager() const
{
    return mNetworkManager;
}

void OrnApiRequest::setNetworkManager(QNetworkAccessManager *networkManager)
{
    if (mNetworkManager != networkManager)
    {
        mNetworkManager = networkManager;
        emit this->networkManagerChanged();
    }
}

void OrnApiRequest::run(const QNetworkRequest &request)
{
    Q_ASSERT_X(mNetworkManager, Q_FUNC_INFO, "networkManager must be set");
    if (mNetworkReply)
    {
        qDebug() << "Request is already running";
        return;
    }
    qDebug() << "Fetching data from" << request.url().toString();
    mNetworkReply = mNetworkManager->get(request);
    connect(mNetworkReply, &QNetworkReply::finished, this, &OrnApiRequest::onReplyFinished);
}

QUrl OrnApiRequest::apiUrl(const QString &resource)
{
    return QUrl(apiUrlPrefix + resource);
}

QNetworkRequest OrnApiRequest::networkRequest()
{
    QNetworkRequest request;
    request.setRawHeader(langName, langValue);
    request.setRawHeader(platformName, platformValue);
    return request;
}

void OrnApiRequest::reset()
{
    if (mNetworkReply)
    {
        mNetworkReply->deleteLater();
        mNetworkReply = 0;
    }
}

void OrnApiRequest::onReplyFinished()
{
    if (mNetworkReply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Network request error" << mNetworkReply->error()
                 << "-" << mNetworkReply->errorString();
        this->reset();
        return;
    }

    QJsonParseError error;
    auto jsonDoc = QJsonDocument::fromJson(mNetworkReply->readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        qCritical() << "Could not parse reply:" << error.errorString();
        this->reset();
        return;
    }

    emit this->jsonReady(jsonDoc);

    this->reset();
}
