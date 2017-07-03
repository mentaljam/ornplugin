#ifndef ORNRECENTAPPSMODEL_H
#define ORNRECENTAPPSMODEL_H

#include "ornabstractappsmodel.h"

class OrnRecentAppsModel : public OrnAbstractAppsModel
{
    Q_OBJECT

public:
    explicit OrnRecentAppsModel(QObject *parent = 0);

    // QAbstractItemModel interface
public:
    void fetchMore(const QModelIndex &parent);
};

#endif // ORNRECENTAPPSMODEL_H
