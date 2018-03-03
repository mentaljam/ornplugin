#include "orncommentlistitem.h"
#include "orn.h"

#include <QJsonObject>


OrnCommentListItem::OrnCommentListItem(const QJsonObject &jsonObject)
    : mCid(Orn::toUint(jsonObject[QStringLiteral("cid")]))
    , mPid(Orn::toUint(jsonObject[QStringLiteral("pid")]))
    , mCreated(Orn::toUint(jsonObject[QStringLiteral("created")]))
    , mText(Orn::toString(jsonObject[QStringLiteral("text")]))
{
    auto user = jsonObject[QStringLiteral("user")].toObject();
    mUserId = Orn::toUint(user[QStringLiteral("uid")]);
    mUserName = Orn::toString(user[QStringLiteral("name")]);
    mUserIconSource = Orn::toString(user[QStringLiteral("picture")].toObject()[QStringLiteral("url")]);
}
