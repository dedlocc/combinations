#pragma once

#include <cstdint>
#include <ctime>
#include <memory>

enum class Period : char
{
    Day = 'd',
    Month = 'm',
    Quarter = 'q',
    Year = 'y',
};

struct Duration
{
    std::size_t amount;
    Period period;
};

class Expiration
{
public:
    Expiration();
    Expiration(const std::tm & tm);

    bool match(Duration dur, const Expiration & target);

    friend bool operator==(const Expiration & d1, const Expiration & d2);
    friend bool operator!=(const Expiration & d1, const Expiration & d2);
    friend bool operator<(const Expiration & d1, const Expiration & d2);
    friend bool operator>(const Expiration & d1, const Expiration & d2);
    friend bool operator<=(const Expiration & d1, const Expiration & d2);
    friend bool operator>=(const Expiration & d1, const Expiration & d2);

private:
    std::size_t year, month, day;
};