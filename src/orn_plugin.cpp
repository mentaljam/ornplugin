#include "orn_plugin.h"
#include "ornapirequest.h"
#include "ornclient.h"
#include "ornpm.h"
#include "ornapplication.h"
#include "ornrecentappsmodel.h"
#include "ornuserappsmodel.h"
#include "ornsearchappsmodel.h"
#include "orncategoryappsmodel.h"
#include "ornrepomodel.h"
#include "orninstalledappsmodel.h"
#include "ornproxymodel.h"
#include "orncommentsmodel.h"
#include "orncategoriesmodel.h"
#include "ornbookmarksmodel.h"
#include "ornbackup.h"

#include "ornapplistitem.h"
#include "orncommentlistitem.h"
#include "orncategorylistitem.h"

#include <qqml.h>

void OrnPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<OrnApiRequest>        (uri, 1, 0, "OrnApiRequest");
    qmlRegisterType<OrnApplication>       (uri, 1, 0, "OrnApplication");
    qmlRegisterType<OrnRecentAppsModel>   (uri, 1, 0, "OrnRecentAppsModel");
    qmlRegisterType<OrnUserAppsModel>     (uri, 1, 0, "OrnUserAppsModel");
    qmlRegisterType<OrnSearchAppsModel>   (uri, 1, 0, "OrnSearchAppsModel");
    qmlRegisterType<OrnCategoryAppsModel> (uri, 1, 0, "OrnCategoryAppsModel");
    qmlRegisterType<OrnRepoModel>         (uri, 1, 0, "OrnRepoModel");
    qmlRegisterType<OrnInstalledAppsModel>(uri, 1, 0, "OrnInstalledAppsModel");
    qmlRegisterType<OrnProxyModel>        (uri, 1, 0, "OrnProxyModel");
    qmlRegisterType<OrnCommentsModel>     (uri, 1, 0, "OrnCommentsModel");
    qmlRegisterType<OrnCategoriesModel>   (uri, 1, 0, "OrnCategoriesModel");
    qmlRegisterType<OrnBookmarksModel>    (uri, 1, 0, "OrnBookmarksModel");
    qmlRegisterType<OrnBackup>            (uri, 1, 0, "OrnBackup");

    qmlRegisterSingletonType<OrnClient>   (uri, 1, 0, "OrnClient", OrnClient::qmlInstance);
    qmlRegisterSingletonType<OrnPm>       (uri, 1, 0, "OrnPm",     OrnPm::qmlInstance);

    qmlRegisterUncreatableType<OrnAppListItem>     (uri, 1, 0, "OrnAppListItem", "uncreatable");
    qmlRegisterUncreatableType<OrnCommentListItem> (uri, 1, 0, "OrnCommentListItem", "uncreatable");
    qmlRegisterUncreatableType<OrnCategoryListItem>(uri, 1, 0, "OrnCategoryListItem", "uncreatable");

    qRegisterMetaType<QList<OrnInstalledPackage>>();
    qRegisterMetaType<QList<OrnPackageVersion>>();
}
