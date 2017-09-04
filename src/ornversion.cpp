#include "ornversion.h"

#include <QRegularExpression>

OrnVersion::OrnVersion()
{

}

OrnVersion::OrnVersion(const QString &string)
{
    auto slist = string.split(QRegularExpression(QStringLiteral("[.+~-]")));
    bool ok;
    for (const QString &s : slist)
    {
        auto v = s.toInt(&ok);
        mParts << (ok ? QVariant(v) : QVariant(s));
    }
}

bool OrnVersion::operator <(const OrnVersion &right)
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
