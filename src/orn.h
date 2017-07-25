#ifndef ORN_H
#define ORN_H

#include <QDateTime>
#include <QMap>
#include <QJsonValue>

namespace PackageKit { class Transaction; }

class Orn
{
public:

    Orn();

    static quint32 toUint(const QJsonValue &value);

    static QString toString(const QJsonValue &value);

    static QDateTime toDateTime(const QJsonValue &value);

    static QList<quint32> toIntList(const QJsonValue &value);

    static PackageKit::Transaction *transaction();
};

#endif // ORN_H
