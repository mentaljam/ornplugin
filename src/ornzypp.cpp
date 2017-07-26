#include "ornzypp.h"

#include <zypp/RepoManager.h>
#include <zypp/ZYppFactory.h>
#include <zypp/target/rpm/RpmDb.h>

#include <QtConcurrent/QtConcurrent>
#include <QSettings>
#include <QLocale>
#include <QFileInfo>

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

#include <QDebug>

#define REPO_BASEURL           QStringLiteral("https://sailfish.openrepos.net/%0/personal/main")
#define SSU_INTERFACE          QStringLiteral("org.nemo.ssu")
#define SSU_PATH               QStringLiteral("/org/nemo/ssu")
#define SSU_METHOD_MODIFYREPO  QStringLiteral("modifyRepo")
#define SSU_METHOD_ADDREPO     QStringLiteral("addRepo")
#define SSU_METHOD_DISPLAYNAME QStringLiteral("displayName")

const QString OrnZypp::repoNamePrefix(QStringLiteral("openrepos-"));
const int OrnZypp::repoNamePrefixLength = OrnZypp::repoNamePrefix.length();

OrnZypp::OrnZypp(QObject *parent) :
    QObject(parent)
{
    zypp::ZYppFactory::instance().getZYpp()->initializeTarget("/");
}

bool OrnZypp::hasRepo(QString alias)
{
    if (!alias.startsWith(repoNamePrefix))
    {
        alias.prepend(repoNamePrefix);
    }
    return zypp::RepoManager().hasRepo(alias.toStdString());
}

bool OrnZypp::repoAction(const QString &author, const RepoAction &action)
{
    auto interface = SSU_INTERFACE;
    bool addRepo = action == AddRepo;
    auto method = addRepo ? SSU_METHOD_ADDREPO : SSU_METHOD_MODIFYREPO;
    auto alias = repoNamePrefix + author;
    auto args = addRepo ? QVariantList{ alias, REPO_BASEURL.arg(author) } :
                          QVariantList{ action, alias };
    auto methodCall = QDBusMessage::createMethodCall(
                interface,
                SSU_PATH,
                interface,
                method);
    methodCall.setArguments(args);
    qDebug() << "Calling" << methodCall;
    auto call = QDBusConnection::systemBus().call(methodCall, QDBus::BlockWithGui, 7000);
    if (!call.errorName().isEmpty())
    {
        qDebug() << call.errorMessage();
        return false;
    }
    return true;
}

QString OrnZypp::deviceModel()
{
    auto interface = SSU_INTERFACE;
    auto methodCall = QDBusMessage::createMethodCall(
                interface,
                SSU_PATH,
                interface,
                SSU_METHOD_DISPLAYNAME);
    // Ssu::DeviceModel = 1
    methodCall.setArguments({ 1 });
    qDebug() << "Calling" << methodCall;
    auto call = QDBusConnection::systemBus().call(methodCall, QDBus::BlockWithGui, 7000);
    return call.arguments().first().toString();
}

void OrnZypp::getInstalledApps()
{
    QtConcurrent::run(this, &OrnZypp::pInstalledApps);
}

void OrnZypp::pInstalledApps()
{
    // Load ORN repositories
    zypp::RepoManager manager;
    auto repos = manager.knownRepositories();
    for (const zypp::RepoInfo &repo : repos)
    {
        auto alias = repo.alias();
        auto qalias = QString::fromStdString(alias);
        if (qalias.startsWith(repoNamePrefix))
        {
            qDebug() << "Loading cache for" << qalias;
            manager.loadFromCache(repo);
        }
    }

    // Load zypp target
    zypp::ZYpp &zypp = *zypp::ZYppFactory::instance().getZYpp();
    zypp.target()->reload();

    // Prepare vars for parsing desktop files
    QString desktop(QStringLiteral(".desktop"));
    QString nameKey(QStringLiteral("Desktop Entry/Name"));
    auto trNameKey = QString(nameKey).append("[%0]");
    auto localeName = QLocale::system().name();
    auto localeNameKey = trNameKey.arg(localeName);
    QString langNameKey;
    if (localeName.length() > 2)
    {
        langNameKey = trNameKey.arg(localeName.left(2));
    }
    QString iconKey(QStringLiteral("Desktop Entry/Icon"));
    QString iconPath(QStringLiteral("/usr/share/icons/hicolor/%0/apps/%1.png"));
    QStringList iconSizes{
        QStringLiteral("86x86"),
        QStringLiteral("108x108"),
        QStringLiteral("128x128"),
        QStringLiteral("256x256")
    };

    // List installed packages
    const auto &proxy = zypp.pool().proxy();
    const auto &rpmDb = zypp.target()->rpmDb();
    AppList apps;
    for (auto it = proxy.byKindBegin(zypp::ResKind::package);
         it != proxy.byKindEnd(zypp::ResKind::package);
         ++it)
    {
        zypp::ui::Selectable::constPtr sel(*it);
        for (auto it2 = sel->picklistBegin(); it2 != sel->picklistEnd(); ++it2)
        {
            zypp::PoolItem pi(*it2);
            auto solv = pi.satSolvable();
            auto repo = solv.repository().info().alias();
            if (repo != "@System" && sel->identicalInstalled(pi))
            {
                qDebug() << "Adding installed package" << QString::fromStdString(solv.asString());
                // Find .desktop file
                zypp::target::rpm::RpmHeader::constPtr header;
                auto name = solv.name();
                auto qname = QString::fromStdString(name);
                auto qtitle = qname;
                auto edition = solv.edition();
                QString qicon;
                rpmDb.getData(name, edition, header);
                for (const auto &f : header->tag_filenames())
                {
                    auto qf = QString::fromStdString(f);
                    if (qf.endsWith(desktop))
                    {
                        qDebug() << "Parsing desktop file" << qf;
                        QSettings desktopFile(qf, QSettings::IniFormat);
                        desktopFile.setIniCodec("UTF-8");
                        // Read pretty name
                        if (desktopFile.contains(localeNameKey))
                        {
                            qtitle = desktopFile.value(localeNameKey).toString();
                        }
                        else if (!langNameKey.isEmpty() && desktopFile.contains(langNameKey))
                        {
                            qtitle = desktopFile.value(langNameKey).toString();
                        }
                        else if (desktopFile.contains(nameKey))
                        {
                            qtitle = desktopFile.value(nameKey).toString();
                        }
                        qDebug() << "Using package name" << qname;
                        // Find icon
                        if (desktopFile.contains(iconKey))
                        {
                            for (const auto &s : iconSizes)
                            {
                                auto qi = iconPath.arg(s, desktopFile.value(iconKey).toString());
                                if (QFileInfo(qi).isFile())
                                {
                                    qDebug() << "Using package icon" << qi;
                                    qicon = qi;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                apps << App{
                    qname,
                    qtitle,
                    QString::fromStdString(edition.asString()),
                    QString::fromStdString(repo).mid(repoNamePrefixLength),
                    qicon
                };
            }
        }
    }

    emit this->installedAppsReady(apps);
}
