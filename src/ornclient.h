#ifndef ORNCLIENT_H
#define ORNCLIENT_H

#include "ornapirequest.h"

class QSettings;
class QTimer;

class OrnClient : public OrnApiRequest
{
    Q_OBJECT
    Q_PROPERTY(bool authorised READ authorised NOTIFY authorisedChanged)
    Q_PROPERTY(bool cookieIsValid READ cookieIsValid NOTIFY cookieIsValidChanged)
    Q_PROPERTY(QString deviceModel READ deviceModel NOTIFY deviceModelChanged)
    Q_PROPERTY(quint32 userId READ userId NOTIFY authorisedChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY authorisedChanged)
    Q_PROPERTY(QString userIconSource READ userIconSource NOTIFY authorisedChanged)

public:
    explicit OrnClient(QObject *parent = 0);

    bool authorised() const;
    bool cookieIsValid() const;
    QString deviceModel() const;
    quint32 userId() const;
    QString userName() const;
    QString userIconSource() const;

public slots:
    void login(const QString &username, const QString &password);
    void logout();
    void comment(const quint32 &appId, const QString &body, const quint32 &parentId = 0);
    void editComment(const quint32 &commentId, const QString &body);

signals:
    void authorisedChanged();
    void deviceModelChanged();
    void authorisationError();
    void dayToExpiry();
    void cookieIsValidChanged();
    void commentAdded(quint32 cid);
    void commentEdited(quint32 cid);

private slots:
    void setCookieTimer();
    void onLoggedIn();
    void onNewComment();
    void onCommentEdited();

private:
    QNetworkRequest authorisedRequest();
    QJsonDocument processReply();

private:
    QSettings *mSettings;
    QTimer *mCookieTimer;
};

#endif // ORNCLIENT_H
