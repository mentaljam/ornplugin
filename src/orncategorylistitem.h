#ifndef ORNCATEGORYLISTITEM_H
#define ORNCATEGORYLISTITEM_H

#include <QObject>

class OrnCategoryListItem : public QObject
{
    friend class OrnCategoriesModel;

    Q_OBJECT
    Q_PROPERTY(quint32 tid MEMBER mTid CONSTANT)
    Q_PROPERTY(quint32 appsCount MEMBER mAppsCount CONSTANT)
    Q_PROPERTY(quint32 depth MEMBER mDepth CONSTANT)
    Q_PROPERTY(QString name MEMBER mName CONSTANT)

public:
    explicit OrnCategoryListItem(QObject *parent = nullptr);
    OrnCategoryListItem(const QJsonObject &jsonObject, QObject *parent = nullptr);

    static QString categoryName(const quint32 &tid);

private:
    static QObjectList parse(const QJsonObject &jsonObject, QObject *parent = nullptr);

private:
    quint32 mTid;
    quint32 mAppsCount;
    quint32 mDepth;
    QString mName;

    static const QMap<quint32, const char*> categories;
};

#endif // ORNCATEGORYLISTITEM_H
