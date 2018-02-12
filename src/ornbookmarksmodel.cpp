#include "ornbookmarksmodel.h"
#include "ornapplication.h"
#include "ornclient.h"

OrnBookmarksModel::OrnBookmarksModel(QObject *parent) :
    OrnAbstractListModel(false, parent)
{
    connect(OrnClient::instance(), &OrnClient::bookmarkChanged,
            this, &OrnBookmarksModel::onBookmarkChanged);
}

void OrnBookmarksModel::onBookmarkChanged(quint32 appId, bool bookmarked)
{
    if (bookmarked)
    {
        this->addApp(appId);
    }
    else
    {
        auto s = mData.size();
        for (int i = 0; i < s; ++i)
        {
            auto app = static_cast<OrnApplication *>(mData[i]);
            if (app->mAppId == appId)
            {
                qDebug() << "Removing app" << appId << "from bookmarks model";
                this->beginRemoveRows(QModelIndex(), i, i);
                mData.removeAt(i);
                this->endRemoveRows();
                app->deleteLater();
                return;
            }
        }
    }
}

void OrnBookmarksModel::addApp(const quint32 &appId)
{
    qDebug() << "Adding app" << appId << "to bookmarks model";
    auto app = new OrnApplication(this);
    app->setAppId(appId);
    connect(app, &OrnApplication::ornRequestFinished, [=]()
    {
        auto s = mData.size();
        this->beginInsertRows(QModelIndex(), s, s);
        mData << app;
        this->endInsertRows();
    });
    app->ornRequest();
}

QVariant OrnBookmarksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    auto app = static_cast<OrnApplication *>(mData[index.row()]);
    switch (role)
    {
    case DataRole:
        return QVariant::fromValue(app);
    case SortRole:
        return app->mTitle;
    case SectionRole:
        return app->mTitle.at(0).toUpper();
    default:
        return QVariant();
    }
}

void OrnBookmarksModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
    {
        return;
    }
    for (const auto &appid : OrnClient::instance()->bookmarks())
    {
        this->addApp(appid);
    }
    mCanFetchMore = false;
}

QHash<int, QByteArray> OrnBookmarksModel::roleNames() const
{
    return {
        { DataRole,    "appData" },
        { SectionRole, "section" }
    };
}
