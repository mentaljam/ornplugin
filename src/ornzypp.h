#ifndef ORNZYPP_H
#define ORNZYPP_H

#include <QObject>

class OrnZypp : public QObject
{
    Q_OBJECT

public:

    /// SSU actions
    enum RepoAction
    {
        RemoveRepo  = 0,
        AddRepo     = 1,
        DisableRepo = 2,
        EnableRepo  = 3
    };

    struct App
    {
        QString name;
        QString title;
        QString version;
        QString author;
        QString icon;
    };

    typedef QList<App> AppList;

    static const QString repoNamePrefix;
    static const int repoNamePrefixLength;

public:
    explicit OrnZypp(QObject *parent = 0);

    static bool hasRepo(QString alias);
    static bool repoAction(const QString &author, const RepoAction &action);
    static QString deviceModel();

signals:
    void installedAppsReady(const AppList &apps);

public slots:
    void getInstalledApps();

private:
    void pInstalledApps();
};

#endif // ORNZYPP_H
