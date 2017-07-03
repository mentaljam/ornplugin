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

const QMap<quint32, const char*> Orn::categories{
    //% "Coding Competition"
    { 3092, QT_TRID_NOOP("orn-cat-coding-competition") },
    //% "Applications"
    {    1, QT_TRID_NOOP("orn-cat-applications") },
    //% "Application"
    {  257, QT_TRID_NOOP("orn-cat-application") },
    //% "Ambience & Themes"
    { 1845, QT_TRID_NOOP("orn-cat-ambience-themes") },
    //% "Business"
    {    2, QT_TRID_NOOP("orn-cat-business") },
    //% "City guides & maps"
    {    3, QT_TRID_NOOP("orn-cat-city-guides-maps") },
    //% "Education & Science"
    { 1324, QT_TRID_NOOP("orn-cat-education-science") },
    //% "Entertainment"
    {    4, QT_TRID_NOOP("orn-cat-entertainment") },
    //% "Music"
    {    5, QT_TRID_NOOP("orn-cat-music") },
    //% "Network"
    {    8, QT_TRID_NOOP("orn-cat-network") },
    //% "News & info"
    {    6, QT_TRID_NOOP("orn-cat-news-info") },
    //% "Patches"
    { 2983, QT_TRID_NOOP("orn-cat-patches") },
    //% "Photo & video"
    {    7, QT_TRID_NOOP("orn-cat-photo-video") },
    //% "Social Networks"
    {    9, QT_TRID_NOOP("orn-cat-social-networks") },
    //% "Sports"
    {   10, QT_TRID_NOOP("orn-cat-sports") },
    //% "System"
    {  147, QT_TRID_NOOP("orn-cat-system") },
    //% "Unknown"
    {  250, QT_TRID_NOOP("orn-cat-unknown") },
    //% "Utilities"
    {   11, QT_TRID_NOOP("orn-cat-utilities") },
    //% "Games"
    {   12, QT_TRID_NOOP("orn-cat-games") },
    //% "Game"
    {  256, QT_TRID_NOOP("orn-cat-game") },
    //% "Action"
    {   13, QT_TRID_NOOP("orn-cat-action") },
    //% "Adventure"
    {   14, QT_TRID_NOOP("orn-cat-adventure") },
    //% "Arcade"
    {   15, QT_TRID_NOOP("orn-cat-arcade") },
    //% "Card & casino"
    {   16, QT_TRID_NOOP("orn-cat-card-casino") },
    //% "Education"
    {   17, QT_TRID_NOOP("orn-cat-education") },
    //% "Puzzle"
    {   18, QT_TRID_NOOP("orn-cat-puzzle") },
    //% "Sports"
    {   19, QT_TRID_NOOP("orn-cat-sports") },
    //% "Strategy"
    {   20, QT_TRID_NOOP("orn-cat-strategy") },
    //% "Trivia"
    {   21, QT_TRID_NOOP("orn-cat-trivia") },
    //% "Translations"
    { 3413, QT_TRID_NOOP("orn-cat-translations") },
    //% "Fonts"
    { 3155, QT_TRID_NOOP("orn-cat-fonts") },
    //% "Libraries"
    {  247, QT_TRID_NOOP("orn-cat-libraries") }
};

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

QString Orn::categoryName(const quint32 &tid)
{
    if (Orn::categories.contains(tid))
    {
        return qtTrId(Orn::categories[tid]);
    }
    else
    {
        qWarning() << "Categories dictionary does not contain tid"
                   << tid << "Dictionary update can be required";
        return qtTrId("orn-cat-unknown");
    }
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
