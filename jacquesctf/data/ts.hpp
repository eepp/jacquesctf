/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TS_HPP
#define _JACQUES_DATA_TS_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <boost/operators.hpp>
#include <yactfr/element.hpp>
#include <yactfr/metadata/clock-type.hpp>

#include "aliases.hpp"
#include "duration.hpp"

namespace jacques {

enum class TsFmtMode
{
    LONG,
    SHORT,
    NS_FROM_ORIGIN,
    CYCLES,
};

enum class Weekday
{
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
};

class Ts final :
    public boost::totally_ordered<Ts>
{
public:
    explicit Ts() noexcept = default;
    explicit Ts(unsigned long long cycles, const yactfr::ClockType& clkType) noexcept;
    explicit Ts(const yactfr::ClockValueElement& elem) noexcept;
    explicit Ts(const yactfr::PacketEndClockValueElement& elem) noexcept;

    explicit Ts(unsigned long long cycles, unsigned long long freq, long long offsetSecs,
                unsigned long long offsetCycles) noexcept;

    Ts(const Ts&) noexcept = default;
    Ts& operator=(const Ts&) noexcept = default;

    unsigned long long cycles() const noexcept
    {
        return _cycles;
    }

    unsigned long long frequency() const noexcept
    {
        return _freq;
    }

    long long nsFromOrigin() const noexcept
    {
        return _nsFromOrigin;
    }

    unsigned int ns() const noexcept
    {
        return _ns;
    }

    unsigned int sec() const noexcept
    {
        return _sec;
    }

    unsigned int min() const noexcept
    {
        return _min;
    }

    unsigned int hour() const noexcept
    {
        return _hour;
    }

    unsigned int day() const noexcept
    {
        return _day;
    }

    unsigned int month() const noexcept
    {
        return _month;
    }

    int year() const noexcept
    {
        return _year;
    }

    Weekday weekday() const noexcept
    {
        return _weekday;
    }

    void format(char *buf, Size bufSize, TsFmtMode fmtMode = TsFmtMode::LONG) const;
    std::string format(TsFmtMode fmtMode = TsFmtMode::LONG) const;

    bool operator==(const Ts& other) const noexcept
    {
        return _nsFromOrigin == other._nsFromOrigin;
    }

    bool operator<(const Ts& other) const noexcept
    {
        return _nsFromOrigin < other._nsFromOrigin;
    }

private:
    unsigned long long _cycles;
    unsigned long long _freq;
    long long _nsFromOrigin;
    unsigned int _ns,
                 _sec,
                 _min,
                 _hour,
                 _day,
                 _month;
    int _year;
    Weekday _weekday;
};

static inline std::ostream& operator<<(std::ostream& stream, const Ts& ts)
{
    std::array<char, 64> buf;

    ts.format(buf.data(), buf.size());
    stream << buf.data();
    return stream;
}

} // namespace jacques

#endif // _JACQUES_DATA_TS_HPP
