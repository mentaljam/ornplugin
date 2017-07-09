#ifndef ORNCATEGORIESMODEL_H
#define ORNCATEGORIESMODEL_H

#include "ornabstractlistmodel.h"

/**
 * @brief The categories model class
 * This will work properly only if api response contains sorted categories
 */
class OrnCategoriesModel : public OrnAbstractListModel
{
    Q_OBJECT
public:
    explicit OrnCategoriesModel(QObject *parent = 0);

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const;
    void fetchMore(const QModelIndex &parent);
    QHash<int, QByteArray> roleNames() const;

    // OrnAbstractListModel interface
protected slots:
    void onJsonReady(const QJsonDocument &jsonDoc);
};

#endif // ORNCATEGORIESMODEL_H
