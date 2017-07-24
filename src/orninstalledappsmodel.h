#ifndef ORNINSTALLEDAPPSMODEL_H
#define ORNINSTALLEDAPPSMODEL_H

#include <QAbstractListModel>

class OrnInstalledAppsModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        VersionRole,
        AuthorRole,
        IconRole,
        SectionRole
    };
    Q_ENUM(Roles)

    explicit OrnInstalledAppsModel(QObject *parent = 0);

public slots:
    void reset();

private:

    struct App
    {
        QString name;
        QString version;
        QString author;
        QString icon;
    };

    QList<App> mData;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};

#endif // ORNINSTALLEDAPPSMODEL_H
