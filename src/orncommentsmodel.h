#ifndef ORNCOMMENTSMODEL_H
#define ORNCOMMENTSMODEL_H

#include "ornabstractlistmodel.h"

class OrnCommentListItem;
class QNetworkReply;

class OrnCommentsModel : public OrnAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(quint32 appId READ appId WRITE setAppId NOTIFY appIdChanged)

public:
    explicit OrnCommentsModel(QObject *parent = 0);

    quint32 appId() const;
    void setAppId(const quint32 &appId);

    Q_INVOKABLE OrnCommentListItem *findItem(const quint32 &cid) const;
    Q_INVOKABLE int findItemRow(const quint32 &cid) const;

public slots:
    void addComment(const quint32 &cid);
    void editComment(const quint32 &cid);

signals:
    void appIdChanged();

private:
    QNetworkReply *fetchComment(const quint32 &cid);
    QJsonObject processReply(QNetworkReply *reply);

private:
    quint32 mAppId;
    QMap<quint32, OrnCommentListItem *> mCommentsMap;

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const;
    void fetchMore(const QModelIndex &parent);
    QHash<int, QByteArray> roleNames() const;
//    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    // OrnAbstractListModel interface
protected slots:
    void onJsonReady(const QJsonDocument &jsonDoc);
};

#endif // ORNCOMMENTSMODEL_H
