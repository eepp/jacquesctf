/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TIMESTAMP_HPP
#define _JACQUES_DATA_TIMESTAMP_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <boost/operators.hpp>
#include <yactfr/element.hpp>
#include <yactfr/metadata/clock-type.hpp>

#include "aliases.hpp"
#include "duration.hpp"

namespace jacques {

enum class TimestampFormatMode
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

class Timestamp :
    public boost::totally_ordered<Timestamp>
{
public:
    Timestamp() = default;
    Timestamp(const Timestamp&) = default;
    explicit Timestamp(unsigned long long cycles, unsigned long long frequency,
                       long long offsetSeconds,
                       unsigned long long offsetCycles);
    explicit Timestamp(unsigned long long cycles,
                       const yactfr::ClockType& clockType);
    explicit Timestamp(const yactfr::ClockValueElement& elem);
    explicit Timestamp(const yactfr::PacketEndClockValueElement& elem);
    Timestamp& operator=(const Timestamp&) = default;

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

    unsigned int second() const noexcept
    {
        return _second;
    }

    unsigned int minute() const noexcept
    {
        return _minute;
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

    void format(char *buf, Size bufSize,
                TimestampFormatMode formatMode = TimestampFormatMode::LONG) const;
    std::string format(TimestampFormatMode formatMode = TimestampFormatMode::LONG) const;

    bool operator==(const Timestamp& other) const noexcept
    {
        return _nsFromOrigin == other._nsFromOrigin;
    }

    bool operator<(const Timestamp& other) const noexcept
    {
        return _nsFromOrigin < other._nsFromOrigin;
    }

private:
    unsigned long long _cycles;
    unsigned long long _freq;
    long long _nsFromOrigin;
    unsigned int _ns,
                 _second,
                 _minute,
                 _hour,
                 _day,
                 _month;
    int _year;
    Weekday _weekday;
};

static inline
std::ostream& operator<<(std::ostream& stream, const Timestamp& ts)
{
    std::array<char, 64> buf;

    ts.format(buf.data(), buf.size());
    stream << buf.data();
    return stream;
}

} // namespace jacques

#endif // _JACQUES_DATA_TIMESTAMP_HPP
