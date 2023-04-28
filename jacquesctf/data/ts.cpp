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
#include <array>
#include <time.h>

#include "ts.hpp"

namespace jacques {

Ts::Ts(const unsigned long long cycles, const unsigned long long freq, long long offsetSecs,
       const unsigned long long offsetCycles) noexcept :
    _cycles {cycles},
    _freq {freq}
{
    assert(offsetCycles < freq);

    const auto secsInCycles = cycles / freq;
    constexpr auto nsInS = 1'000'000'000ULL;
    constexpr auto llNsInS = static_cast<long long>(nsInS);

    offsetSecs += secsInCycles;

    const auto reducedCycles = cycles - secsInCycles * freq;
    constexpr auto maxUll = std::numeric_limits<unsigned long long>::max();
    unsigned long long offsetNsPart;

    if (offsetCycles + reducedCycles <= maxUll / nsInS) {
        offsetNsPart = (offsetCycles + reducedCycles) * nsInS / freq;
    } else {
        /*
         * FIXME: There could be a 1-ns error here, because we're
         * converting to nanoseconds two times (offset and value) and
         * adding those results: if the (real) results were to be 45.6
         * ns and 12.7 ns, for example, which are in fact 45 and 12
         * because of the integer divisions, then the result should be
         * 58 ns, not 57 ns.
         */
        offsetNsPart = (offsetCycles * nsInS / freq) + (reducedCycles * nsInS / freq);
    }

    _nsFromOrigin = offsetSecs * llNsInS + static_cast<long long>(offsetNsPart);

    static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit `time_t`.");

    // I'm pretty sure there's a way to do this without branching
    const auto secsFloor = static_cast<time_t>((_nsFromOrigin < 0) ?
                                               (_nsFromOrigin - llNsInS) / llNsInS :
                                               _nsFromOrigin / llNsInS);

    tm tm;

    localtime_r(&secsFloor, &tm);

    // this too
    if (_nsFromOrigin < 0) {
        _ns = std::abs((std::abs(_nsFromOrigin) % llNsInS) - llNsInS);
    } else {
        _ns = _nsFromOrigin % llNsInS;
    }

    _sec = tm.tm_sec;
    _min = tm.tm_min;
    _hour = tm.tm_hour;
    _day = tm.tm_mday;
    _month = tm.tm_mon + 1;
    _year = 1900 + tm.tm_year;
    _weekday = static_cast<Weekday>(tm.tm_wday);
}

Ts::Ts(const unsigned long long cycles, const yactfr::ClockType& clkType) noexcept :
    Ts {
        cycles,
        clkType.frequency(),
        clkType.offsetFromOrigin().seconds(),
        clkType.offsetFromOrigin().cycles()
    }
{
}

void Ts::format(char * const buf, const Size bufSize, const TsFmtMode fmtMode) const
{
    switch (fmtMode) {
    case TsFmtMode::LONG:
        std::snprintf(buf, bufSize, "%d-%02u-%02u %02u:%02u:%02u.%09u", _year, _month, _day, _hour,
                      _min, _sec, _ns);
        break;

    case TsFmtMode::SHORT:
        std::snprintf(buf, bufSize, "%02u:%02u:%02u.%09u", _hour, _min, _sec, _ns);
        break;

    case TsFmtMode::NS_FROM_ORIGIN:
        std::snprintf(buf, bufSize, "%lld", _nsFromOrigin);
        break;

    case TsFmtMode::CYCLES:
        std::snprintf(buf, bufSize, "%llu", _cycles);
        break;
    }
}

std::string Ts::format(const TsFmtMode fmtMode) const
{
    std::array<char, 64> buf;

    this->format(buf.data(), buf.size(), fmtMode);
    return buf.data();
}

} // namespace jacques
