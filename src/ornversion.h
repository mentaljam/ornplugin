#ifndef ORNVERSION_H
#define ORNVERSION_H

#include <QStringList>

class OrnVersion
{
public:
    explicit OrnVersion(const QString &string);

    inline bool operator ==(const OrnVersion &right)
    {
        return mParts == right.mParts;
    }

    bool operator <(const OrnVersion &right);

    inline bool operator >(const OrnVersion &right)
    {
        return !this->operator <(right);
    }

private:
    QStringList mParts;
};

#endif // ORNVERSION_H
