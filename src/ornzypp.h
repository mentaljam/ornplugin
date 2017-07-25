#ifndef ORNZYPP_H
#define ORNZYPP_H

#include <QObject>

class OrnZypp : public QObject
{
    Q_OBJECT

public:

    struct App
    {
        QString name;
        QString title;
        QString version;
        QString author;
        QString icon;
    };

    typedef QList<App> AppList;

public:
    explicit OrnZypp(QObject *parent = 0);

signals:
    void installedAppsReady(const AppList &apps);

public slots:
    void getInstalledApps();

private:
    void pInstalledApps();
};

#endif // ORNZYPP_H
