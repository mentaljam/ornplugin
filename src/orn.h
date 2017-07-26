#ifndef ORN_H
#define ORN_H

#include <QDateTime>
#include <QMap>
#include <QJsonValue>

namespace PackageKit { class Transaction; }

namespace Orn
{

quint32 toUint(const QJsonValue &value);

QString toString(const QJsonValue &value);

QDateTime toDateTime(const QJsonValue &value);

QList<quint32> toIntList(const QJsonValue &value);

PackageKit::Transaction *transaction();

} // namespace Orn

#endif // ORN_H
