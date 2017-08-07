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

/// Source: https://stackoverflow.com/a/7351507
QByteArray gUncompress(const QByteArray &data);

} // namespace Orn

#endif // ORN_H
