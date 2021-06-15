#include "Date.h"

namespace {
void addMonth(const int amount, int & month, int & year)
{
    month += amount;
    if (month >= 12) {
        year += month / 12;
        month %= 12;
    }
}
} // namespace

Expiration::Expiration() = default;

Expiration::Expiration(const std::tm & tm)
    : year(tm.tm_year)
    , month(tm.tm_mon)
    , day(tm.tm_mday)
{
}

bool Expiration::match(Duration dur, const Expiration & target)
{
    auto temp = *this;

    switch (dur.period) {
    case Period::Quarter:
        addMonth(dur.amount * 3, temp.month, temp.year);
        if (target < temp) {
            return false;
        }
        addMonth(3, temp.month, temp.year);
        return target <= temp;
    case Period::Year:
        temp.year += dur.amount;
        break;
    case Period::Month:
        temp.month += dur.amount;
        break;
    case Period::Day:
        temp.day += dur.amount;
        break;
    }

    std::tm tm{};
    tm.tm_year = temp.year;
    tm.tm_mon = temp.month;
    tm.tm_mday = temp.day;
    std::mktime(&tm);
    return tm == target;
}

bool operator==(const Expiration & d1, const Expiration & d2)
{
    return d1.year == d2.year && d1.month == d2.month && d1.day == d2.day;
}

bool operator!=(const Expiration & d1, const Expiration & d2)
{
    return !(d1 == d2);
}

bool operator<(const Expiration & d1, const Expiration & d2)
{
    if (d1.year == d2.year) {
        if (d1.month == d2.month) {
            return d1.day < d2.day;
        }
        return d1.month < d2.month;
    }
    return d1.year < d2.year;
}

bool operator>(const Expiration & d1, const Expiration & d2)
{
    return d2 < d1;
}

bool operator<=(const Expiration & d1, const Expiration & d2)
{
    return !(d1 > d2);
}

bool operator>=(const Expiration & d1, const Expiration & d2)
{
    return !(d1 < d2);
}
