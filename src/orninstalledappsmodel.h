#ifndef ORNINSTALLEDAPPSMODEL_H
#define ORNINSTALLEDAPPSMODEL_H

#include <QAbstractListModel>

#include "ornzypp.h"

class OrnInstalledAppsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(OrnZypp* zypp READ zypp WRITE setZypp NOTIFY zyppChanged)

public:

    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        TitleRole,
        VersionRole,
        AuthorRole,
        IconRole,
        SectionRole
    };
    Q_ENUM(Roles)

    explicit OrnInstalledAppsModel(QObject *parent = 0);

    OrnZypp *zypp() const;
    void setZypp(OrnZypp *zypp);

signals:
    void zyppChanged();

public slots:
    void reset();

private slots:
    void onInstalledAppsReady(const OrnZypp::AppList &apps);

private:
    OrnZypp *mZypp;
    OrnZypp::AppList mData;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};

#endif // ORNINSTALLEDAPPSMODEL_H
