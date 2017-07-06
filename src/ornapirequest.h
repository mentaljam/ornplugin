#ifndef ORNAPIREQUEST_H
#define ORNAPIREQUEST_H

#include <QNetworkAccessManager>

class OrnApiRequest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool networkError READ networkError NOTIFY networkErrorChanged)
    Q_PROPERTY(QNetworkAccessManager* networkManager READ networkManager WRITE setNetworkManager NOTIFY networkManagerChanged)

public:
    explicit OrnApiRequest(QObject *parent = 0);
    ~OrnApiRequest();

    bool networkError() const;

    QNetworkAccessManager *networkManager() const;
    void setNetworkManager(QNetworkAccessManager *networkManager);

    void run(const QNetworkRequest &request);

    static QUrl apiUrl(const QString &resource);
    static QNetworkRequest networkRequest();

public slots:
    void reset();

signals:
    void networkManagerChanged();
    void jsonReady(const QJsonDocument &jsonDoc);
    void networkErrorChanged();

private slots:
    void onReplyFinished();

private:
    void setNetworkError(bool error);

protected:
    bool mNetworkError;
    QNetworkAccessManager *mNetworkManager;
    QNetworkReply *mNetworkReply;

private:
    static const QString apiUrlPrefix;
    static const QByteArray langName;
    static const QByteArray langValue;
    static const QByteArray platformName;
    static const QByteArray platformValue;
};

#endif // ORNAPIREQUEST_H
