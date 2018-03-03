#ifndef ORNCOMMENTLISTITEM_H
#define ORNCOMMENTLISTITEM_H


#include <QObject>
#include <QString>

class QJsonObject;

class OrnCommentListItem : public QObject
{
    friend class OrnCommentsModel;

    Q_OBJECT
    Q_PROPERTY(quint32 commentId MEMBER mCid CONSTANT)
    Q_PROPERTY(quint32 parentId MEMBER mPid CONSTANT)
    Q_PROPERTY(quint32 created MEMBER mCreated CONSTANT)
    Q_PROPERTY(quint32 userId MEMBER mUserId CONSTANT)
    Q_PROPERTY(QString userName MEMBER mUserName CONSTANT)
    Q_PROPERTY(QString userIconSource MEMBER mUserIconSource CONSTANT)
    Q_PROPERTY(QString text MEMBER mText CONSTANT)

public:
    OrnCommentListItem(const QJsonObject &jsonObject);

private:
    quint32 mCid;
    quint32 mPid;
    quint32 mCreated;
    quint32 mUserId;
    QString mUserName;
    QString mUserIconSource;
    QString mText;
};

#endif // ORNCOMMENTLISTITEM_H
