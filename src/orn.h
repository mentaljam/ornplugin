#ifndef ORN_H
#define ORN_H

#include <QDateTime>
#include <QJsonValue>

namespace Orn
{

quint32 toUint(const QJsonValue &value);

QString toString(const QJsonValue &value);

QDateTime toDateTime(const QJsonValue &value);

QList<quint32> toIntList(const QJsonValue &value);

} // namespace Orn

#endif // ORN_H
