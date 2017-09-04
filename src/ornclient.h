#ifndef ORNCLIENT_H
#define ORNCLIENT_H

#include "ornapirequest.h"

#include <QSet>
#include <QVariant>

class QSettings;
class QTimer;
class QQmlEngine;
class QJSEngine;

class OrnClient : public OrnApiRequest
{
    friend class OrnBackup;

    Q_OBJECT
    Q_PROPERTY(bool authorised READ authorised NOTIFY authorisedChanged)
    Q_PROPERTY(bool cookieIsValid READ cookieIsValid NOTIFY cookieIsValidChanged)
    Q_PROPERTY(quint32 userId READ userId NOTIFY authorisedChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY authorisedChanged)
    Q_PROPERTY(QString userIconSource READ userIconSource NOTIFY authorisedChanged)
    Q_PROPERTY(QList<quint32> bookmarks READ bookmarks NOTIFY bookmarksChanged)

public:
    explicit OrnClient(QObject *parent = 0);
    ~OrnClient();

    static OrnClient *instance();
    static inline QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        return OrnClient::instance();
    }

    bool authorised() const;
    bool cookieIsValid() const;
    quint32 userId() const;
    QString userName() const;
    QString userIconSource() const;

    QList<quint32> bookmarks() const;
    Q_INVOKABLE bool hasBookmark(const quint32 &appId) const;
    Q_INVOKABLE bool addBookmark(const quint32 &appId);
    Q_INVOKABLE bool removeBookmark(const quint32 &appId);

    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant value(const QString &key) const;

public slots:
    void login(const QString &username, const QString &password);
    void logout();
    void comment(const quint32 &appId, const QString &body, const quint32 &parentId = 0);
    void editComment(const quint32 &commentId, const QString &body);

signals:
    void authorisedChanged();
    void authorisationError();
    void dayToExpiry();
    void cookieIsValidChanged();
    void commentAdded(quint32 cid);
    void commentEdited(quint32 cid);
    void bookmarksChanged();
    void bookmarkChanged(quint32 appid, bool bookmarked);

private slots:
    void setCookieTimer();
    void onLoggedIn();
    void onNewComment();
    void onCommentEdited();

private:
    QNetworkRequest authorisedRequest();
    QJsonDocument processReply();
    static void prepareComment(QJsonObject &object, const QString &body);

private:
    QSettings *mSettings;
    QTimer *mCookieTimer;
    QSet<quint32> mBookmarks;

    static OrnClient *gInstance;
};

#endif // ORNCLIENT_H
