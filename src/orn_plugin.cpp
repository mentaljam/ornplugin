#include "orn_plugin.h"
#include "ornapirequest.h"
#include "ornapplication.h"
#include "ornapplistitem.h"
#include "orncommentlistitem.h"
#include "ornrecentappsmodel.h"
#include "ornuserappsmodel.h"
#include "ornsearchappsmodel.h"
#include "ornrepomodel.h"
#include "ornproxymodel.h"
#include "orncommentsmodel.h"

#include <qqml.h>

void OrnPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<OrnApiRequest>      (uri, 1, 0, "OrnApiRequest");
    qmlRegisterType<OrnApplication>     (uri, 1, 0, "OrnApplication");
    qmlRegisterType<OrnAppListItem>     (uri, 1, 0, "OrnAppListItem");
    qmlRegisterType<OrnCommentListItem> (uri, 1, 0, "OrnCommentListItem");
    qmlRegisterType<OrnRecentAppsModel> (uri, 1, 0, "OrnRecentAppsModel");
    qmlRegisterType<OrnUserAppsModel>   (uri, 1, 0, "OrnUserAppsModel");
    qmlRegisterType<OrnSearchAppsModel> (uri, 1, 0, "OrnSearchAppsModel");
    qmlRegisterType<OrnRepoModel>       (uri, 1, 0, "OrnRepoModel");
    qmlRegisterType<OrnProxyModel>      (uri, 1, 0, "OrnProxyModel");
    qmlRegisterType<OrnCommentsModel>   (uri, 1, 0, "OrnCommentsModel");
}
