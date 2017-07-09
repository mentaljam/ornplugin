#include "orn.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QDebug>

const QString Orn::repoNamePrefix(QStringLiteral("openrepos-"));
const QString Orn::repoUrlTemplate(QStringLiteral("http://sailfish.openrepos.net/%0/personal/main"));
const QString Orn::repoFileTemplate(QStringLiteral("/etc/zypp/repos.d/ssu_%0_release.repo"));

const QString Orn::ssuInterface(QStringLiteral("org.nemo.ssu"));
const QString Orn::ssuPath(QStringLiteral("/org/nemo/ssu"));
const QString Orn::ssuModifyRepoMethod(QStringLiteral("modifyRepo"));
const QString Orn::ssuAddRepoMethod(QStringLiteral("addRepo"));

Orn::Orn()
{

}

quint32 Orn::toUint(const QJsonValue &value)
{
    return value.toString().remove(QChar(',')).toUInt();
}

QString Orn::toString(const QJsonValue &value)
{
    return value.toString().trimmed();
}

QDateTime Orn::toDateTime(const QJsonValue &value)
{
    return QDateTime::fromMSecsSinceEpoch((quint64)toUint(value) * 1000);
}

QList<quint32> Orn::toIntList(const QJsonValue &value)
{
    auto array = value.toArray();
    QString tidKey(QStringLiteral("tid"));
    QList<quint32> list;
    for (const QJsonValue &v: array)
    {
        list << Orn::toUint(v.toObject()[tidKey]);
    }
    return list;
}

bool Orn::isRepoInstalled(QString userId)
{
    if (!userId.startsWith(repoNamePrefix))
    {
        userId.prepend(repoNamePrefix);
    }
    return QFile(repoFileTemplate.arg(userId)).exists();
}

bool Orn::addRepo(const QString &userName)
{
    auto methodCall = QDBusMessage::createMethodCall(
                ssuInterface,
                ssuPath,
                ssuInterface,
                ssuAddRepoMethod);
    methodCall.setArguments({
                                repoNamePrefix + userName,
                                repoUrlTemplate.arg(userName)
                            });
    qDebug() << "Calling" << methodCall;
    auto call = QDBusConnection::systemBus().call(methodCall, QDBus::BlockWithGui, 7000);
    return call.errorName().isEmpty();
}

bool Orn::modifyRepo(const QString &userName, const RepoAction &action)
{
    auto methodCall = QDBusMessage::createMethodCall(
                ssuInterface,
                ssuPath,
                ssuInterface,
                ssuModifyRepoMethod);
    methodCall.setArguments({
                                action,
                                repoNamePrefix + userName
                            });
    qDebug() << "Calling" << methodCall;
    auto call = QDBusConnection::systemBus().call(methodCall, QDBus::BlockWithGui, 7000);
    return call.errorName().isEmpty();
}
