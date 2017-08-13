#include "orn.h"

#include <QJsonArray>
#include <QJsonObject>

#include <QDebug>

namespace Orn
{

quint32 toUint(const QJsonValue &value)
{
    return value.toString().remove(QChar(',')).toUInt();
}

QString toString(const QJsonValue &value)
{
    return value.toString().trimmed();
}

QDateTime toDateTime(const QJsonValue &value)
{
    return QDateTime::fromMSecsSinceEpoch((quint64)toUint(value) * 1000);
}

QList<quint32> toIntList(const QJsonValue &value)
{
    auto array = value.toArray();
    QString tidKey(QStringLiteral("tid"));
    QList<quint32> list;
    for (const QJsonValue &v: array)
    {
        list << toUint(v.toObject()[tidKey]);
    }
    return list;
}

} // namespace Orn
