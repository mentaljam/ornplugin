#include "orn.h"

#include <PackageKit/packagekit-qt5/Transaction>

#include <zlib.h>

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
        qDebug() << t << "finished in" << runtime << "msec" << "with status" << status;
        t->deleteLater();
    });
#ifdef QT_DEBUG
    QObject::connect(t, &PackageKit::Transaction::errorCode,
                     [=](PackageKit::Transaction::Error error, const QString &details)
    {
        qDebug() << "An error occured while running" << t << ":" << error << "-" << details;
    });
#endif
    return t;
}

QByteArray gUncompress(const QByteArray &data)
{
    QByteArray result;

    if (data.size() <= 4)
    {
        qWarning("Input data is truncated");
        return result;
    }

    // Allocate inflate state
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    // GZip decoding
    auto ret = inflateInit2(&strm, 15 + 16);
    if (ret != Z_OK)
    {
        return result;
    }

    // Run inflate()
    static const int chunkSize = 1024;
    char out[chunkSize];
    do
    {
        strm.avail_out = chunkSize;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);

        Q_ASSERT(ret != Z_STREAM_ERROR);
        switch (ret)
        {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, chunkSize - strm.avail_out);
    }
    while (strm.avail_out == 0);

    // Clean up and return
    inflateEnd(&strm);
    return result;
}

} // namespace Orn
