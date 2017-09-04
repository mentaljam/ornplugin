#ifndef ORNBOOKMARKSMODEL_H
#define ORNBOOKMARKSMODEL_H

#include "ornabstractlistmodel.h"

class OrnBookmarksModel : public OrnAbstractListModel
{
    Q_OBJECT

public:

    enum Roles
    {
        DataRole = Qt::UserRole,
        SortRole,
        SectionRole
    };
    Q_ENUM(Roles)

    explicit OrnBookmarksModel(QObject *parent = 0);

private slots:
    void onBookmarkChanged(quint32 appId, bool bookmarked);
    void addApp(const quint32 &appId);

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const;
    void fetchMore(const QModelIndex &parent);
    QHash<int, QByteArray> roleNames() const;

    // OrnAbstractListModel interface
protected slots:
    void onJsonReady(const QJsonDocument &jsonDoc) { Q_UNUSED(jsonDoc) }
};

#endif // ORNBOOKMARKSMODEL_H
