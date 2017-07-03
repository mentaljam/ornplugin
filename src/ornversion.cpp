#include "ornversion.h"

#include <QRegularExpression>

OrnVersion::OrnVersion(const QString &string) :
    mParts(string.split(QRegularExpression(QStringLiteral("[.+~-]"))))
{

}

bool OrnVersion::operator <(const OrnVersion &right)
{
    auto leftLength = mParts.length();
    auto rightLength = right.mParts.length();
    auto leftIsShorter = leftLength < rightLength;
    auto shorterLength = leftIsShorter ? leftLength : rightLength;

    // Compare the same parts
    for (int i = 0; i < shorterLength; ++i)
    {
        auto comp = mParts[i].compare(right.mParts[i]);
        if (comp != 0)
        {
            return comp < 0;
        }
    }

    // If the same parts are equal then the shorter version is lesser
    return leftIsShorter;
}
