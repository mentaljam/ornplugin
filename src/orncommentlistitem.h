#ifndef ORNCOMMENTLISTITEM_H
#define ORNCOMMENTLISTITEM_H

#include <QObject>

class OrnCommentListItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString userName MEMBER mUserName CONSTANT)
    Q_PROPERTY(QString userIconSource MEMBER mUserIconSource CONSTANT)
    Q_PROPERTY(QString text MEMBER mText CONSTANT)
    Q_PROPERTY(QString date MEMBER mDate CONSTANT)

public:
    explicit OrnCommentListItem(QObject *parent = 0);
    OrnCommentListItem(const QJsonObject &jsonObject, QObject *parent = 0);

private:
    quint32 mCreated;
    QString mUserName;
    QString mUserIconSource;
    QString mText;
    QString mDate;
};

#endif // ORNCOMMENTLISTITEM_H
