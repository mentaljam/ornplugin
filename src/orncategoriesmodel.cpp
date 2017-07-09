#include "orncategoriesmodel.h"
#include "orncategorylistitem.h"
#include "ornapirequest.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

OrnCategoriesModel::OrnCategoriesModel(QObject *parent) :
    OrnAbstractListModel(false, parent)
{

}

QVariant OrnCategoriesModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    if (!index.isValid())
    {
        return QVariant();
    }
    return QVariant::fromValue(mData[index.row()]);
}

void OrnCategoriesModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
    {
        return;
    }
    OrnAbstractListModel::apiCall(QStringLiteral("categories"));
}

QHash<int, QByteArray> OrnCategoriesModel::roleNames() const
{
    return { { Qt::DisplayRole, "categoryData" } };
}

void OrnCategoriesModel::onJsonReady(const QJsonDocument &jsonDoc)
{
    this->reset();
    mCanFetchMore = false;
    auto categoriesArray = jsonDoc.array();
    if (categoriesArray.isEmpty())
    {
        qWarning() << "Api reply is empty";
        return;
    }

    QObjectList list;
    for (const auto &category: categoriesArray)
    {
        list << OrnCategoryListItem::parse(category.toObject(), this);
    }
    this->beginInsertRows(QModelIndex(), 0, list.size() - 1);
    mData = list;
    qDebug() << list.size() << "items have been added to the model";
    this->endInsertRows();
    emit this->replyProcessed();
}

