#include "orn.h"

#include <PackageKit/packagekit-qt5/Transaction>

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
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

PackageKit::Transaction *transaction()
{
    auto t = new PackageKit::Transaction();
    QObject::connect(t, &PackageKit::Transaction::finished,
                     [=](PackageKit::Transaction::Exit status, uint runtime)
    {
        qDebug() << "Transaction" << t->uid() << "finished in"
                 << runtime << "msec" << "with status" << status;
        t->deleteLater();
    });
#ifndef NDEBUG
    QObject::connect(t, &PackageKit::Transaction::errorCode,
                     [=](PackageKit::Transaction::Error error, const QString &details)
    {
        qDebug() << "An error occured while running transaction" << t->uid()
                 << ":" << error << "-" << details;
    });
#endif
    return t;
}

} // namespace Orn
