#ifndef ORNAPPLISTITEM_H
#define ORNAPPLISTITEM_H

#include <QObject>
#include <QDate>

class QJsonObject;

class OrnAppListItem : public QObject
{
    friend class OrnAbstractAppsModel;
    friend class OrnBookmarksModel;

    Q_OBJECT

    Q_PROPERTY(quint32 appId MEMBER mAppId CONSTANT)
    Q_PROPERTY(quint32 created MEMBER mCreated CONSTANT)
    Q_PROPERTY(QDate createDate READ createDate CONSTANT)
    Q_PROPERTY(quint32 updated MEMBER mUpdated CONSTANT)
    Q_PROPERTY(quint32 ratingCount MEMBER mRatingCount CONSTANT)
    Q_PROPERTY(float   rating MEMBER mRating CONSTANT)
    Q_PROPERTY(QString title MEMBER mTitle CONSTANT)
    Q_PROPERTY(QString userName MEMBER mUserName CONSTANT)
    Q_PROPERTY(QString iconSource MEMBER mIconSource CONSTANT)
    Q_PROPERTY(QString sinceUpdate MEMBER mSinceUpdate CONSTANT)
    Q_PROPERTY(QString category MEMBER mCategory CONSTANT)
    Q_PROPERTY(QString package MEMBER mPackage CONSTANT)

public:
    OrnAppListItem(const QJsonObject &jsonObject);

    QDate createDate() const;

private:
    static QString sinceLabel(const quint32 &value);

private:
    quint32 mAppId;
    quint32 mCreated;
    quint32 mUpdated;
    quint32 mRatingCount;
    float mRating;
    QString mTitle;
    QString mUserName;
    QString mIconSource;
    QString mSinceUpdate;
    QString mCategory;
    QString mPackage;
};

#endif // ORNAPPLISTITEM_H
