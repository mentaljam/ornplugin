#include "orninstalledappsmodel.h"
#include "orn.h"

#include <QDebug>

OrnInstalledAppsModel::OrnInstalledAppsModel(QObject *parent) :
    QAbstractListModel(parent),
    mZypp(0)
{

}

OrnZypp *OrnInstalledAppsModel::zypp() const
{
    return mZypp;
}

void OrnInstalledAppsModel::setZypp(OrnZypp *zypp)
{
    if (mZypp != zypp)
    {
        qDebug() << zypp;
        if (mZypp)
        {
            disconnect(mZypp, &OrnZypp::installedAppsReady,
                       this, &OrnInstalledAppsModel::onInstalledAppsReady);
        }
        mZypp = zypp;
        if (mZypp)
        {
            connect(mZypp, &OrnZypp::installedAppsReady,
                    this, &OrnInstalledAppsModel::onInstalledAppsReady);
            this->reset();
        }
        emit this->zyppChanged();
    }
}

void OrnInstalledAppsModel::reset()
{
    if (!mZypp)
    {
        qCritical() << "Zypp should be set";
        return;
    }
    qDebug() << "Resetting model";
    this->beginResetModel();
    mZypp->getInstalledApps();
}

void OrnInstalledAppsModel::onInstalledAppsReady(const OrnZypp::AppList &apps)
{
    if (!apps.empty())
    {
        mData.clear();
        mData.append(apps);
    }
    else
    {
        qWarning() << "App list is empty";
    }
    this->endResetModel();
}

int OrnInstalledAppsModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? mData.size() : 0;
}

QVariant OrnInstalledAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    auto app = mData[index.row()];
    switch (role)
    {
    case NameRole:
        return app.name;
    case TitleRole:
        return app.title;
    case VersionRole:
        return app.version;
    case AuthorRole:
        return app.author;
    case IconRole:
        return app.icon;
    case SectionRole:
        return app.title.at(0).toUpper();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OrnInstalledAppsModel::roleNames() const
{
    return {
        { NameRole,    "appName"       },
        { TitleRole,   "appTitle"      },
        { VersionRole, "appVersion"    },
        { AuthorRole,  "appAuthor"     },
        { IconRole,    "appIconSource" },
        { SectionRole, "section"       }
    };
}
