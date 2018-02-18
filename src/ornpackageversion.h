#ifndef ORNPACKAGEVERSION_H
#define ORNPACKAGEVERSION_H


#include <QVariantList>

struct OrnPackageVersion
{
    quint64 downloadSize;
    quint64 installSize;
    QString version;
    QString arch;
    QString repoAlias;

    OrnPackageVersion();
    OrnPackageVersion(const QString &version);
    OrnPackageVersion(const quint64 &dsize, const quint64 &isize,
                      const QString &version, const QString &arch, const QString &alias);

    QString packageId(const QString &name) const;

    bool operator ==(const OrnPackageVersion &other) const;
    inline bool operator !=(const OrnPackageVersion &other) const
    { return !this->operator ==(other); }

    bool operator <(const OrnPackageVersion &other) const;

private:
    QVariantList versionParts;
};

typedef QList<OrnPackageVersion> OrnPackageVersionList;

Q_DECLARE_METATYPE(QList<OrnPackageVersion>)

#endif // ORNPACKAGEVERSION_H
