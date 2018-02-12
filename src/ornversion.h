#ifndef ORNVERSION_H
#define ORNVERSION_H

#include <QVariantList>

class OrnVersion
{
public:
    OrnVersion();
    explicit OrnVersion(const QString &string);
    OrnVersion(const OrnVersion &other);

    QString toString() const;

    void operator =(const OrnVersion &other);

    bool operator ==(const OrnVersion &right) const;
    bool operator <(const OrnVersion &right) const;
    inline bool operator >(const OrnVersion &right) const
    {
        return !this->operator <(right);
    }

private:
    QString mOrigin;
    QVariantList mParts;
};

#endif // ORNVERSION_H
