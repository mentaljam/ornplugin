#ifndef ORNAPIREQUEST_H
#define ORNAPIREQUEST_H

#include <QObject>

class QNetworkReply;
class QNetworkRequest;

class OrnApiRequest : public QObject
{
    Q_OBJECT

public:
    explicit OrnApiRequest(QObject *parent = 0);
    ~OrnApiRequest();

    void run(const QNetworkRequest &request);

    static QUrl apiUrl(const QString &resource);
    static QNetworkRequest networkRequest();

public slots:
    void reset();

signals:
    void jsonReady(const QJsonDocument &jsonDoc);

protected slots:
    void onReplyFinished();

protected:
    QNetworkReply *mNetworkReply;

private:
    static const QString apiUrlPrefix;
    static const QByteArray langName;
    static const QByteArray langValue;
    static const QByteArray platformName;
    static const QByteArray platformValue;
};

#endif // ORNAPIREQUEST_H
