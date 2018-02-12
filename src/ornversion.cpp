#include "ornversion.h"

#include <QRegularExpression>

OrnVersion::OrnVersion()
{

}

OrnVersion::OrnVersion(const QString &string)
    : mOrigin(string)
{
    auto slist = string.split(QRegularExpression(QStringLiteral("[.+~-]")));
    bool ok;
    for (const QString &s : slist)
    {
        auto v = s.toInt(&ok);
        mParts << (ok ? QVariant(v) : QVariant(s));
    }
}

OrnVersion::OrnVersion(const OrnVersion &other)
    : mOrigin(other.mOrigin)
    , mParts(other.mParts)
{

}

QString OrnVersion::toString() const
{
    return mOrigin;
}

void OrnVersion::operator =(const OrnVersion &other)
{
    mOrigin = other.mOrigin;
    mParts = other.mParts;
}

bool OrnVersion::operator ==(const OrnVersion &right) const
{
    return mOrigin == right.mOrigin;
}

bool OrnVersion::operator <(const OrnVersion &right) const
{
    if (mParts == right.mParts)
    {
        return false;
    }

    auto leftLength = mParts.length();
    auto rightLength = right.mParts.length();
    auto leftIsShorter = leftLength < rightLength;
    auto shorterLength = leftIsShorter ? leftLength : rightLength;

    // Compare the same parts
    for (int i = 0; i < shorterLength; ++i)
    {
        const auto &lp = mParts[i];
        const auto &rp = right.mParts[i];
        if (lp != rp)
        {
            return lp < rp;
        }
    }

    // If the same parts are equal then the shorter version is lesser
    return leftIsShorter;
}
