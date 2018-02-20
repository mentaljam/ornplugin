#include "orncommentlistitem.h"
#include "orn.h"

#include <QJsonObject>

OrnCommentListItem::OrnCommentListItem(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This constructor is only for moc");
}

OrnCommentListItem::OrnCommentListItem(const QJsonObject &jsonObject, QObject *parent) :
    QObject(parent),
    mCid(Orn::toUint(jsonObject[QStringLiteral("cid")])),
    mPid(Orn::toUint(jsonObject[QStringLiteral("pid")])),
    mCreated(Orn::toUint(jsonObject[QStringLiteral("created")])),
    mText(Orn::toString(jsonObject[QStringLiteral("text")]))
{
    auto user = jsonObject[QStringLiteral("user")].toObject();
    mUserId = Orn::toUint(user[QStringLiteral("uid")]);
    mUserName = Orn::toString(user[QStringLiteral("name")]);
    mUserIconSource = Orn::toString(user[QStringLiteral("picture")].toObject()[QStringLiteral("url")]);
}
