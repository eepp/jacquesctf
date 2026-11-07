/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdlib>
#include <limits>
#include <time.h>
#include <fmt/format.h>

#include "ts.hpp"

namespace jacques {
namespace {

constexpr auto nsPerSec = 1'000'000'000LL;

/*
 * Returns `cycles` × 10⁹ / `freq` exactly, without overflowing, where
 * `cycles` is less than `freq`.
 */
unsigned long long cyclesToNs(unsigned long long cycles, const unsigned long long freq) noexcept
{
    assert(cycles < freq);

    constexpr auto ullNsPerSec = static_cast<unsigned long long>(nsPerSec);

    if (cycles <= std::numeric_limits<unsigned long long>::max() / ullNsPerSec) {
        // common case: no overflow
        return cycles * ullNsPerSec / freq;
    }

    // long division, one decimal digit at a time
    unsigned long long ns = 0;

    for (auto i = 0; i < 9; ++i) {
        cycles *= 10;
        ns = ns * 10 + cycles / freq;
        cycles %= freq;
    }

    return ns;
}

} // namespace

Ts::Ts(const unsigned long long cycles, const unsigned long long freq, const long long offsetSecs,
       const unsigned long long offsetCycles, const bool hasUnixEpochOrigin) noexcept :
    _cycles {cycles},
    _freq {freq},
    _hasUnixEpochOrigin {hasUnixEpochOrigin}
{
    assert(offsetCycles < freq);

    // both terms are less than `freq`
    const auto subSecCycles = offsetCycles + cycles % freq;

    const auto secs = offsetSecs + static_cast<long long>(cycles / freq + subSecCycles / freq);

    _nsFromOrigin = secs * nsPerSec +
                    static_cast<long long>(cyclesToNs(subSecCycles % freq, freq));

    // floor division: the nanosecond part is never negative
    auto secsFloor = _nsFromOrigin / nsPerSec;
    auto ns = _nsFromOrigin % nsPerSec;

    if (ns < 0) {
        ns += nsPerSec;
        --secsFloor;
    }

    _ns = ns;

    if (!_hasUnixEpochOrigin) {
        // calendar properties are meaningless without a known origin
        return;
    }

    static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit `time_t`.");

    const auto timeT = static_cast<time_t>(secsFloor);
    tm tm;

    localtime_r(&timeT, &tm);
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
        clkType.offsetFromOrigin().cycles(),
        clkType.origin() && clkType.origin()->isUnixEpoch()
    }
{
}

TsFmtMode Ts::effectiveFmtMode(const TsFmtMode fmtMode) const noexcept
{
    if (!_hasUnixEpochOrigin && (fmtMode == TsFmtMode::LONG || fmtMode == TsFmtMode::SHORT)) {
        // can't format a date without a known origin!
        return TsFmtMode::NS_FROM_ORIGIN;
    }

    return fmtMode;
}

std::string Ts::format(const TsFmtMode fmtMode) const
{
    switch (this->effectiveFmtMode(fmtMode)) {
    case TsFmtMode::LONG:
        return fmt::format("{}-{:02}-{:02} {:02}:{:02}:{:02}.{:09}", _year, _month, _day, _hour,
                           _min, _sec, _ns);

    case TsFmtMode::SHORT:
        return fmt::format("{:02}:{:02}:{:02}.{:09}", _hour, _min, _sec, _ns);

    case TsFmtMode::NS_FROM_ORIGIN:
        return fmt::to_string(_nsFromOrigin);

    case TsFmtMode::CYCLES:
        return fmt::to_string(_cycles);
    }

    std::abort();
}

} // namespace jacques
