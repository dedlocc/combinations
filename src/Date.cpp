#include "Date.h"

namespace {
void addMonth(const std::size_t amount, std::size_t & month, std::size_t & year)
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
        year += dur.amount;
        break;
    case Period::Month:
        addMonth(dur.amount, temp.month, temp.year);
        break;
    case Period::Day:
        std::tm tm;
        tm.tm_year = year;
        tm.tm_mon = month;
        tm.tm_mday = day + dur.amount;
        std::mktime(&tm);
        year = tm.tm_year;
        month = tm.tm_mon;
        day = tm.tm_mday;
        break;
    }

    return temp == target;
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
