/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <limits>
#include <curses.h>
#include <time.h>

#include "data/timestamp.hpp"

namespace jacques {

Timestamp::Timestamp(const unsigned long long cycles,
                     const unsigned long long frequency,
                     long long offsetSeconds,
                     const unsigned long long offsetCycles) :
    _cycles {cycles},
    _freq {frequency}
{
    assert(offsetCycles < frequency);

    const auto secondsInCycles = cycles / frequency;
    constexpr auto nsInS = 1'000'000'000ULL;
    constexpr auto llNsInS = static_cast<long long>(nsInS);

    offsetSeconds += secondsInCycles;

    const auto reducedCycles = cycles - secondsInCycles * frequency;
    constexpr auto maxUll = std::numeric_limits<unsigned long long>::max();
    unsigned long long offsetNsPart;

    if (offsetCycles + reducedCycles <= maxUll / nsInS) {
        offsetNsPart = (offsetCycles + reducedCycles) * nsInS / frequency;
    } else {
        /*
         * FIXME: There could be a 1-ns error here, because we're
         * converting to nanoseconds two times (offset and value) and
         * adding those results: if the (real) results were to be 45.6
         * ns and 12.7 ns, for example, which are in fact 45 and 12
         * because of the integer divisions, then the result should be
         * 58 ns, not 57 ns.
         */
        offsetNsPart = (offsetCycles * nsInS / frequency) +
                       (reducedCycles * nsInS / frequency);
    }

    _nsFromOrigin = offsetSeconds * llNsInS +
                    static_cast<long long>(offsetNsPart);

    time_t secondsFloor;

    static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit time_t.");

    // I'm pretty sure there's a way to do this without branching
    if (_nsFromOrigin < 0) {
        secondsFloor = static_cast<time_t>((_nsFromOrigin - llNsInS) / llNsInS);
    } else {
        secondsFloor = static_cast<time_t>(_nsFromOrigin / llNsInS);
    }

    tm tm;

    localtime_r(&secondsFloor, &tm);

    // this too
    if (_nsFromOrigin < 0) {
        _ns = std::abs((std::abs(_nsFromOrigin) % llNsInS) - llNsInS);
    } else {
        _ns = _nsFromOrigin % llNsInS;
    }

    _second = tm.tm_sec;
    _minute = tm.tm_min;
    _hour = tm.tm_hour;
    _day = tm.tm_mday;
    _month = tm.tm_mon + 1;
    _year = 1900 + tm.tm_year;
    _weekday = static_cast<Weekday>(tm.tm_wday);
}

Timestamp::Timestamp(unsigned long long cycles,
                     const yactfr::ClockType& clockType) :
    Timestamp {
        cycles, clockType.freq(),
        clockType.offset().seconds(),
        clockType.offset().cycles(),
    }
{
}

Timestamp::Timestamp(const yactfr::ClockValueElement& elem) :
    Timestamp {elem.cycles(), elem.clockType()}
{
}

Timestamp::Timestamp(const yactfr::PacketEndClockValueElement& elem) :
    Timestamp {elem.cycles(), elem.clockType()}
{
}

void Timestamp::format(char * const buf, const Size bufSize,
                       const TimestampFormatMode formatMode) const
{
    switch (formatMode) {
    case TimestampFormatMode::LONG:
        std::snprintf(buf, bufSize, "%d-%02u-%02u %02u:%02u:%02u.%09u",
                      _year, _month, _day, _hour, _minute, _second, _ns);
        break;

    case TimestampFormatMode::SHORT:
        std::snprintf(buf, bufSize, "%02u:%02u:%02u.%09u",
                      _hour, _minute, _second, _ns);
        break;

    case TimestampFormatMode::NS_FROM_ORIGIN:
        std::snprintf(buf, bufSize, "%lld", _nsFromOrigin);
        break;

    case TimestampFormatMode::CYCLES:
        std::snprintf(buf, bufSize, "%llu", _cycles);
        break;
    }
}

std::string Timestamp::format(const TimestampFormatMode formatMode) const
{
    std::array<char, 64> buf;

    this->format(buf.data(), buf.size(), formatMode);
    return buf.data();
}

} // namespace jacques
