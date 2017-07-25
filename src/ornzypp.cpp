#include "ornzypp.h"
#include "orn.h"

#include <zypp/RepoManager.h>
#include <zypp/ZYppFactory.h>
#include <zypp/target/rpm/RpmDb.h>

#include <QtConcurrent/QtConcurrent>
#include <QSettings>
#include <QLocale>
#include <QFileInfo>
#include <QDebug>

OrnZypp::OrnZypp(QObject *parent) :
    QObject(parent)
{

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
        if (qalias.startsWith(Orn::repoNamePrefix))
        {
            qDebug() << "Loading cache for" << qalias;
            manager.loadFromCache(repo);
        }
    }

    // Load zypp target
    zypp::ZYpp &zypp = *zypp::ZYppFactory::instance().getZYpp();
    zypp.initializeTarget("/");
    zypp.target()->load();

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
                    QString::fromStdString(repo).mid(Orn::repoNamePrefixLength),
                    qicon
                };
            }
        }
    }

    emit this->installedAppsReady(apps);
}
