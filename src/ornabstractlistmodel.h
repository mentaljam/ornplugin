#ifndef ORNABSTRACTLISTMODEL_H
#define ORNABSTRACTLISTMODEL_H

#include <QAbstractListModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class OrnApiRequest;

class OrnAbstractListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(OrnApiRequest* apiRequest READ apiRequest CONSTANT)

public:
    OrnAbstractListModel(bool fetchable, QObject *parent = 0);

    OrnApiRequest *apiRequest() const;

public slots:
    void reset();

protected:
    void apiCall(const QString &resource);
    template<typename T>
    void processReply(const QJsonDocument &jsonDoc)
    {
        auto jsonArray = jsonDoc.array();
        if (jsonArray.size() > 0)
        {
            QObjectList list;
            for (const QJsonValueRef &jsonValue: jsonArray)
            {
                // Each class of list item should implement a constructor
                // SomeListItem(const QJsonValue &, QObject *)
                list << new T(jsonValue.toObject(), this);
            }
            auto row = mData.size();
            this->beginInsertRows(QModelIndex(), row, row + list.size() - 1);
            mData.append(list);
            this->endInsertRows();
            ++mPage;
            qDebug() << list.size() << "items have been added to the model";
            if (!mFetchable)
            {
                mCanFetchMore = false;
            }
        }
        else
        {
            qDebug() << "Reply is empty, the model has fetched all data";
            mCanFetchMore = false;
        }
    }

protected slots:
    virtual void onJsonReady(const QJsonDocument &jsonDoc) = 0;

protected:
    bool    mFetchable;
    bool    mCanFetchMore;
    quint32 mPage;
    QObjectList mData;
    OrnApiRequest *mApiRequest;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    virtual void fetchMore(const QModelIndex &parent) = 0;
    virtual bool canFetchMore(const QModelIndex &parent) const = 0;
};

#endif // ORNABSTRACTLISTMODEL_H
