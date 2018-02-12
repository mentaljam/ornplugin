#ifndef ORNINSTALLEDPACKAGE_H
#define ORNINSTALLEDPACKAGE_H


#include <QMetaType>

struct OrnInstalledPackage
{
    bool updateAvailable;
    QString name;
    QString title;
    QString version;
    QString icon;
};

typedef QList<OrnInstalledPackage> OrnInstalledPackageList;

Q_DECLARE_METATYPE(QList<OrnInstalledPackage>)

#endif // ORNINSTALLEDPACKAGE_H
