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
#include <curses.h>
#include <time.h>

#include "timestamp.hpp"

namespace jacques {

Timestamp::Timestamp(const unsigned long long cycles,
                     const unsigned long long frequency,
                     long long offsetSeconds,
                     const unsigned long long offsetCycles) :
    _cycles {cycles},
    _freq {frequency}
{
    auto llCycles = static_cast<long long>(cycles);
    const auto llFreq = static_cast<long long>(frequency);
    auto llOffsetCycles = static_cast<long long>(offsetCycles);
    const auto llSecondsInOffsetCycles = llOffsetCycles / llFreq;
    const auto llSecondsInCycles = llCycles / llFreq;
    constexpr auto nsInS = 1'000'000'000LL;

    offsetSeconds += llSecondsInOffsetCycles;
    llOffsetCycles -= llSecondsInOffsetCycles * llFreq;
    offsetSeconds += llSecondsInCycles;
    llCycles -= llSecondsInCycles * llFreq;

    assert(llCycles >= 0);
    assert(llFreq >= 0);
    assert(llOffsetCycles >= 0);

    const auto offsetNs = (llOffsetCycles * nsInS) / llFreq +
                          offsetSeconds * nsInS;
    _nsFromEpoch = offsetNs + (llCycles * nsInS) / llFreq;

    time_t secondsFloor;

    static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit time_t.");

    // I'm pretty sure there's a way to do this without branching
    if (_nsFromEpoch < 0) {
        secondsFloor = static_cast<time_t>((_nsFromEpoch - nsInS) / nsInS);
    } else {
        secondsFloor = static_cast<time_t>(_nsFromEpoch / nsInS);
    }

    tm tm;

    localtime_r(&secondsFloor, &tm);

    // this too
    if (_nsFromEpoch < 0) {
        _ns = std::abs((std::abs(_nsFromEpoch) % nsInS) - nsInS);
    } else {
        _ns = _nsFromEpoch % nsInS;
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

    case TimestampFormatMode::NS_FROM_EPOCH:
        std::snprintf(buf, bufSize, "%lld", _nsFromEpoch);
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
