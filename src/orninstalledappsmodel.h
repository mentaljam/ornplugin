#ifndef ORNINSTALLEDAPPSMODEL_H
#define ORNINSTALLEDAPPSMODEL_H

#include <QAbstractListModel>

#include "ornzypp.h"

class OrnInstalledAppsModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        TitleRole,
        VersionRole,
        AuthorRole,
        IconRole,
        SortRole,
        SectionRole,
        UpdateAvailableRole
    };
    Q_ENUM(Roles)

    explicit OrnInstalledAppsModel(QObject *parent = 0);

public slots:
    void reset();

private slots:
    void onInstalledAppsReady(const OrnZypp::AppList &apps);

private:
    OrnZypp::AppList mData;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};

#endif // ORNINSTALLEDAPPSMODEL_H
