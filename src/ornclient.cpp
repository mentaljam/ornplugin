#include "ornclient.h"
#include "orn.h"

#include <QSettings>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#define DEVICE_MODEL        QStringLiteral("device")
#define USER_COOKIE         QStringLiteral("user/cookie")
#define USER_COOKIE_EXPIRE  QStringLiteral("user/cookie/expire")
#define USER_TOKEN          QStringLiteral("user/token")
#define USER_UID            QStringLiteral("user/uid")
#define USER_NAME           QStringLiteral("user/name")
#define USER_REALNAME       QStringLiteral("user/realname")
#define USER_MAIL           QStringLiteral("user/mail")
#define USER_CREATED        QStringLiteral("user/created")
#define USER_PICTURE        QStringLiteral("user/picture/url")

#define APPLICATION_JSON    QByteArrayLiteral("application/json")

OrnClient::OrnClient(QObject *parent) :
    OrnApiRequest(parent),
    mSettings(new QSettings(this)),
    mCookieTimer(new QTimer(this))
{
    // Get device model
    QString deviceKey(DEVICE_MODEL);
    if (!mSettings->contains(deviceKey))
    {
        mSettings->setValue(deviceKey, Orn::deviceModel());
        emit this->deviceModelChanged();
    }
    qDebug() << "Device model is" << this->deviceModel();

    // Configure cookie timer
    mCookieTimer->setSingleShot(true);
    this->setCookieTimer();

    // Check if authorisation has expired
    connect(this, &OrnClient::networkManagerChanged, [=]()
    {
        if (mNetworkManager && this->authorised())
        {
            qDebug() << "Checking authorisation status";
            auto request = this->authorisedRequest();
            request.setUrl(OrnClient::apiUrl(QStringLiteral("session")));
            mNetworkReply = mNetworkManager->get(request);
            connect(mNetworkReply, &QNetworkReply::finished, [=]()
            {
#ifndef NDEBUG
                if (this->processReply().object().contains(QStringLiteral("token")))
                {
                    qDebug() << "Client is authorised";
                }
#else
                this->processReply();
#endif
                this->reset();
            });
        }
    });
}

bool OrnClient::authorised() const
{
    return mSettings->contains(USER_TOKEN) &&
           mSettings->contains(USER_COOKIE);
}

bool OrnClient::cookieIsValid() const
{
    QString cookieExpireKey(USER_COOKIE_EXPIRE);
    return mSettings->contains(cookieExpireKey) &&
           mSettings->value(cookieExpireKey).toDateTime() > QDateTime::currentDateTime();
}

QString OrnClient::deviceModel() const
{
    return mSettings->value(DEVICE_MODEL).toString();
}

quint32 OrnClient::userId() const
{
    return mSettings->value(USER_UID).toUInt();
}

QString OrnClient::userName() const
{
    return mSettings->value(USER_NAME).toString();
}

QString OrnClient::userIconSource() const
{
    return mSettings->value(USER_PICTURE).toString();
}

void OrnClient::login(const QString &username, const QString &password)
{
    Q_ASSERT_X(mNetworkManager, Q_FUNC_INFO, "networkManager must be set");

    // Stop timer and remove old credentials
    this->setCookieTimer();
    mSettings->remove(QStringLiteral("user"));

    QNetworkRequest request;
    request.setUrl(OrnApiRequest::apiUrl(QStringLiteral("user/login")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, APPLICATION_JSON);

    QJsonObject jsonObject;
    jsonObject.insert(QStringLiteral("username"), username);
    jsonObject.insert(QStringLiteral("password"), password);
    QJsonDocument jsonDoc(jsonObject);

    mNetworkReply = mNetworkManager->post(request, jsonDoc.toJson());
    connect(mNetworkReply, &QNetworkReply::finished, this, &OrnClient::onLoggedIn);
}

void OrnClient::logout()
{
    if (this->authorised())
    {
        this->setCookieTimer();
        mSettings->remove(QStringLiteral("user"));
        emit this->authorisedChanged();
    }
}

void OrnClient::comment(const quint32 &appId, const QString &body, const quint32 &parentId)
{
    Q_ASSERT_X(mNetworkManager, Q_FUNC_INFO, "networkManager must be set");

    auto request = this->authorisedRequest();
    request.setUrl(OrnApiRequest::apiUrl(QStringLiteral("comments")));

    QJsonObject commentObject;
    commentObject.insert(QStringLiteral("appid"), QString::number(appId));
    if (parentId != 0)
    {
        commentObject.insert(QStringLiteral("pid"), QString::number(parentId));
    }
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("value"), body);
    QJsonArray undArray;
    undArray.append(bodyObject);
    QJsonObject undObject;
    undObject.insert(QStringLiteral("und"), undArray);
    commentObject.insert(QStringLiteral("comment_body"), undObject);
    QJsonDocument jsonDoc(commentObject);

    mNetworkReply = mNetworkManager->post(request, jsonDoc.toJson());
    connect(mNetworkReply, &QNetworkReply::finished, this, &OrnClient::onNewComment);
}

void OrnClient::editComment(const quint32 &commentId, const QString &body)
{
    Q_ASSERT_X(mNetworkManager, Q_FUNC_INFO, "networkManager must be set");

    auto request = this->authorisedRequest();
    request.setUrl(OrnApiRequest::apiUrl(QStringLiteral("comments/%0").arg(commentId)));

    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("value"), body);
    QJsonArray undArray;
    undArray.append(bodyObject);
    QJsonObject undObject;
    undObject.insert(QStringLiteral("und"), undArray);
    QJsonObject commentObject;
    commentObject.insert(QStringLiteral("comment_body"), undObject);
    QJsonDocument jsonDoc(commentObject);

    mNetworkReply = mNetworkManager->put(request, jsonDoc.toJson());
    connect(mNetworkReply, &QNetworkReply::finished, this, &OrnClient::onCommentEdited);
}

void OrnClient::setCookieTimer()
{
    disconnect(mCookieTimer, &QTimer::timeout, 0, 0);
    mCookieTimer->stop();
    QString cookieExpireKey(USER_COOKIE_EXPIRE);
    if (mSettings->contains(cookieExpireKey))
    {
        auto msecToExpire = mSettings->value(cookieExpireKey).toDateTime()
                .msecsTo(QDateTime::currentDateTime());
        if (msecToExpire > 86400000)
        {
            connect(mCookieTimer, &QTimer::timeout, this, &OrnClient::dayToExpiry);
            mCookieTimer->start(msecToExpire - 86400000);
        }
        else if (msecToExpire > 0)
        {
            emit this->dayToExpiry();
            connect(mCookieTimer, &QTimer::timeout, this, &OrnClient::cookieIsValidChanged);
            mCookieTimer->start(msecToExpire);
        }
        else
        {
            emit this->cookieIsValidChanged();
        }
    }
}

void OrnClient::onLoggedIn()
{
    auto jsonDoc = this->processReply();
    if (jsonDoc.isEmpty())
    {
        emit this->authorisationError();
        return;
    }
    auto cookieVariant = mNetworkReply->header(QNetworkRequest::SetCookieHeader);
    if (cookieVariant.isValid() && jsonDoc.isObject())
    {
        auto jsonObject = jsonDoc.object();

        auto cookie = cookieVariant.value<QList<QNetworkCookie> >().first();
        mSettings->setValue(USER_COOKIE, cookie.toRawForm(QNetworkCookie::NameAndValueOnly));
        mSettings->setValue(USER_COOKIE_EXPIRE, cookie.expirationDate());

        mSettings->setValue(USER_TOKEN, Orn::toString(jsonObject[QStringLiteral("token")]));

        jsonObject = jsonObject[QStringLiteral("user")].toObject();
        mSettings->setValue(USER_UID, Orn::toUint(jsonObject[QStringLiteral("uid")]));
        mSettings->setValue(USER_NAME, Orn::toString(jsonObject[QStringLiteral("name")]));
        mSettings->setValue(USER_MAIL, Orn::toString(jsonObject[QStringLiteral("mail")]));
        mSettings->setValue(USER_CREATED, Orn::toDateTime(jsonObject[QStringLiteral("created")]));
        mSettings->setValue(USER_PICTURE, jsonObject[QStringLiteral("picture")]
                .toObject()[QStringLiteral("url")].toString());

        QString undKey(QStringLiteral("und"));
        QString valueKey(QStringLiteral("value"));
        auto name = jsonObject[QStringLiteral("field_name")].toObject()[undKey]
                .toArray().first().toObject()[valueKey].toString();
        auto surname = jsonObject[QStringLiteral("field_surname")].toObject()[undKey]
                .toArray().first().toObject()[valueKey].toString();
        auto hasName = !name.isEmpty();
        auto hasSurname = !surname.isEmpty();
        auto fullname = hasName && hasSurname ? name.append(" ").append(surname) :
                                                hasName ? name : hasSurname ? surname : QString();
        mSettings->setValue(USER_REALNAME, fullname);

        qDebug() << "Successful authorisation";
        emit this->authorisedChanged();
        this->setCookieTimer();
    }
    this->reset();
}

void OrnClient::onNewComment()
{
    auto jsonDoc = this->processReply();
    if (jsonDoc.isObject())
    {
        auto cid = Orn::toUint(jsonDoc.object()[QStringLiteral("cid")]);
        emit this->commentAdded(cid);
        qDebug() << "Comment added:" << cid;
    }
    this->reset();
}

void OrnClient::onCommentEdited()
{
    auto jsonDoc = this->processReply();
    if (jsonDoc.isArray())
    {
        auto cid = Orn::toUint(jsonDoc.array().first());
        emit this->commentEdited(cid);
        qDebug() << "Comment edited:" << cid;
    }
    this->reset();
}

QNetworkRequest OrnClient::authorisedRequest()
{
    Q_ASSERT(this->authorised());
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, APPLICATION_JSON);
    request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(
                      QNetworkCookie::parseCookies(mSettings->value(USER_COOKIE).toByteArray()).first()));
    request.setRawHeader(QByteArrayLiteral("X-CSRF-Token"), mSettings->value(USER_TOKEN).toByteArray());
    return request;
}

QJsonDocument OrnClient::processReply()
{
    auto networkError = mNetworkReply->error();
    if (networkError != QNetworkReply::NoError)
    {
        qDebug() << "Network request error" << mNetworkReply->error()
                 << "-" << mNetworkReply->errorString();
        if (this->authorised() && networkError == QNetworkReply::ServiceUnavailableError)
        {
            emit this->cookieIsValidChanged();
        }
        return QJsonDocument();
    }

    QJsonParseError error;
    auto jsonDoc = QJsonDocument::fromJson(mNetworkReply->readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        qCritical() << "Could not parse reply:" << error.errorString();
    }

    return jsonDoc;
}