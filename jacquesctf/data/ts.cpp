/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdlib>
#include <limits>
#include <time.h>
#include <fmt/chrono.h>
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
    const auto effFmtMode = this->effectiveFmtMode(fmtMode);

    switch (effFmtMode) {
    case TsFmtMode::LONG:
    case TsFmtMode::SHORT:
    {
        // floor division: the nanosecond part is never negative
        auto secs = _nsFromOrigin / nsPerSec;
        auto ns = _nsFromOrigin % nsPerSec;

        if (ns < 0) {
            ns += nsPerSec;
            --secs;
        }

        static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit `time_t`.");

        const auto timeT = static_cast<time_t>(secs);
        tm tm;

        localtime_r(&timeT, &tm);

        if (effFmtMode == TsFmtMode::LONG) {
            return fmt::format("{:%F %T}.{:09}", tm, ns);
        }

        return fmt::format("{:%T}.{:09}", tm, ns);
    }

    case TsFmtMode::NS_FROM_ORIGIN:
        return fmt::to_string(_nsFromOrigin);

    case TsFmtMode::CYCLES:
        return fmt::to_string(_cycles);
    }

    std::abort();
}

} // namespace jacques
