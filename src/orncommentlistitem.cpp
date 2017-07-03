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
    mCreated(Orn::toUint(jsonObject[QStringLiteral("created")])),
    mText(Orn::toString(jsonObject[QStringLiteral("text")])),
    mDate(QDateTime::fromMSecsSinceEpoch((quint64)mCreated * 1000).date().toString(Qt::SystemLocaleLongDate))
{
    // Comment text contains a trailing empty line
    mText = mText.left(mText.length() - 1);

    auto user = jsonObject[QStringLiteral("user")].toObject();
    mUserName = Orn::toString(user[QStringLiteral("name")]);
    mUserIconSource = Orn::toString(user[QStringLiteral("picture")].toObject()[QStringLiteral("url")]);
}
