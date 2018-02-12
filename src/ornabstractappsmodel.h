#ifndef ORNABSTRACTAPPSMODEL_H
#define ORNABSTRACTAPPSMODEL_H

#include "ornabstractlistmodel.h"

class OrnAbstractAppsModel : public OrnAbstractListModel
{
    Q_OBJECT

public:

    enum Roles
    {
        DataRole = Qt::DisplayRole,
        TitleRole = Qt::UserRole,
        DateRole
    };
    Q_ENUM(Roles)

    OrnAbstractAppsModel(bool fetchable, QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    // OrnAbstractListModel interface
protected slots:
    void onJsonReady(const QJsonDocument &jsonDoc);
};

#endif // ORNABSTRACTAPPSMODEL_H
