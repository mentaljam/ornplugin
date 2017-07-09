#ifndef ORN_H
#define ORN_H

#include <QDateTime>
#include <QMap>
#include <QJsonValue>

class Orn
{
public:

    enum RepoAction
    {
        RemoveRepo  = 0,
        AddRepo     = 1,
        DisableRepo = 2,
        EnableRepo  = 3
    };

    static const QString repoNamePrefix;
    static const QString repoUrlTemplate;
    static const QString repoFileTemplate;

    static const QString ssuInterface;
    static const QString ssuPath;
    static const QString ssuModifyRepoMethod;
    static const QString ssuAddRepoMethod;

public:

    Orn();

    static quint32 toUint(const QJsonValue &value);

    static QString toString(const QJsonValue &value);

    static QDateTime toDateTime(const QJsonValue &value);

    static QList<quint32> toIntList(const QJsonValue &value);

    static bool isRepoInstalled(QString userId);
    static bool addRepo(const QString &userName);
    static bool modifyRepo(const QString &userName, const RepoAction &action);
};

#endif // ORN_H
