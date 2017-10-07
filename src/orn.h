#ifndef ORN_H
#define ORN_H

#include <QDateTime>
#include <QJsonValue>

namespace Orn
{

inline quint32 toUint(const QJsonValue &value)
{
    return value.toString().remove(QChar(',')).toUInt();
}

inline QString toString(const QJsonValue &value)
{
    return value.toString().trimmed();
}

inline QDateTime toDateTime(const QJsonValue &value)
{
    return QDateTime::fromMSecsSinceEpoch(quint64(toUint(value)) * 1000);
}

QList<quint32> toIntList(const QJsonValue &value);

QString locate(const QString &filename);

} // namespace Orn

#endif // ORN_H
