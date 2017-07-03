#ifndef ORN_PLUGIN_H
#define ORN_PLUGIN_H

#include <QQmlExtensionPlugin>

class OrnPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri = "harbour.orn");
};

#endif // ORN_PLUGIN_H
