#include "orn_plugin.h"
#include "ornapirequest.h"
#include "ornclient.h"
#include "ornapplication.h"
#include "ornapplistitem.h"
#include "orncommentlistitem.h"
#include "orncategorylistitem.h"
#include "ornrecentappsmodel.h"
#include "ornuserappsmodel.h"
#include "ornsearchappsmodel.h"
#include "orncategoryappsmodel.h"
#include "ornrepomodel.h"
#include "orninstalledappsmodel.h"
#include "ornproxymodel.h"
#include "orncommentsmodel.h"
#include "orncategoriesmodel.h"
#include "ornzypp.h"

#include <qqml.h>

void OrnPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<OrnApiRequest>        (uri, 1, 0, "OrnApiRequest");
    qmlRegisterType<OrnClient>            (uri, 1, 0, "OrnClient");
    qmlRegisterType<OrnApplication>       (uri, 1, 0, "OrnApplication");
    qmlRegisterType<OrnAppListItem>       (uri, 1, 0, "OrnAppListItem");
    qmlRegisterType<OrnCommentListItem>   (uri, 1, 0, "OrnCommentListItem");
    qmlRegisterType<OrnCategoryListItem>  (uri, 1, 0, "OrnCategoryListItem");
    qmlRegisterType<OrnRecentAppsModel>   (uri, 1, 0, "OrnRecentAppsModel");
    qmlRegisterType<OrnUserAppsModel>     (uri, 1, 0, "OrnUserAppsModel");
    qmlRegisterType<OrnSearchAppsModel>   (uri, 1, 0, "OrnSearchAppsModel");
    qmlRegisterType<OrnCategoryAppsModel> (uri, 1, 0, "OrnCategoryAppsModel");
    qmlRegisterType<OrnRepoModel>         (uri, 1, 0, "OrnRepoModel");
    qmlRegisterType<OrnInstalledAppsModel>(uri, 1, 0, "OrnInstalledAppsModel");
    qmlRegisterType<OrnProxyModel>        (uri, 1, 0, "OrnProxyModel");
    qmlRegisterType<OrnCommentsModel>     (uri, 1, 0, "OrnCommentsModel");
    qmlRegisterType<OrnCategoriesModel>   (uri, 1, 0, "OrnCategoriesModel");

    qRegisterMetaType<OrnZypp::AppList>("AppList");
}
