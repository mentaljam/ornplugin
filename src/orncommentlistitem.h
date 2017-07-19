#ifndef ORNCOMMENTLISTITEM_H
#define ORNCOMMENTLISTITEM_H

#include <QObject>

class QTimer;

class OrnCommentListItem : public QObject
{
    friend class OrnCommentsModel;

    Q_OBJECT
    Q_PROPERTY(quint32 commentId MEMBER mCid CONSTANT)
    Q_PROPERTY(quint32 parentId MEMBER mPid CONSTANT)
    Q_PROPERTY(quint32 userId MEMBER mUserId CONSTANT)
    Q_PROPERTY(QString userName MEMBER mUserName CONSTANT)
    Q_PROPERTY(QString userIconSource MEMBER mUserIconSource CONSTANT)
    Q_PROPERTY(QString text MEMBER mText CONSTANT)
    Q_PROPERTY(QString date MEMBER mDate CONSTANT)

public:
    explicit OrnCommentListItem(QObject *parent = 0);
    OrnCommentListItem(const QJsonObject &jsonObject, QObject *parent = 0);

private:
    static QString sinceCreated(const quint64 &created);

private:
    quint32 mCid;
    quint32 mPid;
    quint32 mCreated;
    quint32 mUserId;
    QString mUserName;
    QString mUserIconSource;
    QString mText;
    QString mDate;
    QTimer  *mCreatedTimer;
};

#endif // ORNCOMMENTLISTITEM_H
