#include "orncommentlistitem.h"
#include "orn.h"

#include <QJsonObject>
#include <QTimer>

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
    mText(Orn::toString(jsonObject[QStringLiteral("text")])),
    mDate(OrnCommentListItem::sinceCreated(mCreated)),
    mCreatedTimer(new QTimer(this))
{
    // Comment text contains a trailing empty line
    mText = mText.left(mText.length() - 1);

    auto user = jsonObject[QStringLiteral("user")].toObject();
    mUserName = Orn::toString(user[QStringLiteral("name")]);
    mUserIconSource = Orn::toString(user[QStringLiteral("picture")].toObject()[QStringLiteral("url")]);

    mCreatedTimer->setSingleShot(false);
    connect(mCreatedTimer, &QTimer::timeout, [=]()
    {
        mDate = OrnCommentListItem::sinceCreated(mCreated);
    });
    mCreatedTimer->start(30000);
}

QString OrnCommentListItem::sinceCreated(const quint64 &created)
{
    auto createdDt = QDateTime::fromMSecsSinceEpoch(created * 1000);
    auto currentDt = QDateTime::currentDateTime();

    auto mins = int(double(createdDt.secsTo(currentDt)) / 60.0 + 0.5);
    if (mins < 1)
    {
        //% "Just now"
        return qtTrId("orn-just-now");
    }
    auto hours = int(double(mins) / 60.0 + 0.5);
    if (hours < 1)
    {
        //% "%0 minute(s) ago"
        return qtTrId("orn-mins-ago", mins).arg(mins);
    }
    auto days = int(double(hours) / 24.0 + 0.5);
    if (days < 1)
    {
        //% "%0 hour(s) ago"
        return qtTrId("orn-hours-ago", hours).arg(hours);
    }
    if (days == 1)
    {
        //% "Yesterday"
        return qtTrId("orn-yesterday");
    }
    return createdDt.date().toString(Qt::SystemLocaleLongDate);
}
