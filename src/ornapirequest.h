#ifndef ORNAPIREQUEST_H
#define ORNAPIREQUEST_H

#include <QNetworkAccessManager>

class OrnApiRequest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QNetworkAccessManager* networkManager READ networkManager WRITE setNetworkManager NOTIFY networkManagerChanged)

public:
    explicit OrnApiRequest(QObject *parent = 0);
    ~OrnApiRequest();

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

protected slots:
    void onReplyFinished();

protected:
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
