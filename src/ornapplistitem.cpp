#include "ornapplistitem.h"
#include "orn.h"
#include "orncategorylistitem.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

OrnAppListItem::OrnAppListItem(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This constructor is only for moc");
}

OrnAppListItem::OrnAppListItem(const QJsonObject &jsonObject, QObject *parent) :
    QObject(parent),
    mAppId(jsonObject[QStringLiteral("appid")].toVariant().toUInt()),
    mUpdated(Orn::toUint(jsonObject[QStringLiteral("updated")])),
    mTitle(Orn::toString(jsonObject[QStringLiteral("title")])),
    mUserName(Orn::toString(jsonObject[QStringLiteral("user")].toObject()[QStringLiteral("name")])),
    mIconSource(Orn::toString(jsonObject[QStringLiteral("icon")].toObject()[QStringLiteral("url")])),
    mSinceUpdate(sinceLabel(mUpdated))
{
    QString ratingKey(QStringLiteral("rating"));
    auto ratingObject = jsonObject[ratingKey].toObject();
    mRatingCount = Orn::toUint(ratingObject[QStringLiteral("count")]);
    mRating = ratingObject[ratingKey].toString().toFloat();

    auto categories = jsonObject[QStringLiteral("category")].toArray();
    auto tid = Orn::toUint(categories.last().toObject()[QStringLiteral("tid")]);
    mCategory = OrnCategoryListItem::categoryName(tid);
//    auto categories = jsonObject[QStringLiteral("category")].toArray();
//    QString tidKey(QStringLiteral("tid"));
//    QStringList list;
//    for (const QJsonValue &c: categories)
//    {
//        auto tid = Orn::toUint(c.toObject()[tidKey]);
//        list << qtTrId(Orn::categories[tid]);
//    }
//    mCategories = list.join(QByteArrayLiteral(" • "));
}

QString OrnAppListItem::sinceLabel(const quint32 &value)
{
    auto curDate = QDate::currentDate();
    auto date = QDateTime::fromMSecsSinceEpoch((quint64)value * 1000).date();
    auto days = date.daysTo(curDate);
    if (days == 0)
    {
        //% "Today"
        return qtTrId("orn-today");
    }
    if (days == 1)
    {
        //% "Yesterday"
        return qtTrId("orn-yesterday");
    }
    if (days < 7 && date.dayOfWeek() < curDate.dayOfWeek())
    {
        //% "This week"
        return qtTrId("orn-this-week");
    }
    if (days < curDate.daysInMonth() && date.day() < curDate.day())
    {
        //% "This month"
        return qtTrId("orn-this-month");
    }
    //: Output format for the month and year - %0 is a long month name and %1 is a year (for example "May 2017")
    //% "%0 %1"
    return qtTrId("orn-month-format").arg(QDate::longMonthName(date.month())).arg(date.year());
}
