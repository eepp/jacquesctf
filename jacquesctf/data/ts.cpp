/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdlib>
#include <limits>
#include <time.h>
#include <utility>
#include <fmt/chrono.h>
#include <fmt/format.h>

#include "ts.hpp"
#include "time-ops.hpp"

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

// local calendar time of `nsFromOrigin` and its nanosecond part
std::pair<tm, long long> localTime(const long long nsFromOrigin) noexcept
{
    // floor division: the nanosecond part is never negative
    auto secs = nsFromOrigin / nsPerSec;
    auto ns = nsFromOrigin % nsPerSec;

    if (ns < 0) {
        ns += nsPerSec;
        --secs;
    }

    static_assert(sizeof(time_t) >= 8, "Expecting a 64-bit `time_t`.");

    const auto timeT = static_cast<time_t>(secs);
    tm tm;

    localtime_r(&timeT, &tm);
    return {tm, ns};
}

// formatted UTC offset of `tm` (see `LongFmtTs::utcOffset`)
std::string fmtUtcOffset(const tm& tm)
{
    auto utcOffset = fmt::format("{:%Ez}", tm);

    if (utcOffset == "+00:00" || utcOffset == "-00:00") {
        return "Z";
    }

    return utcOffset;
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

FmtTs Ts::format(const TsFmtMode fmtMode) const
{
    const auto effFmtMode = this->effectiveFmtMode(fmtMode);

    switch (effFmtMode) {
    case TsFmtMode::LONG:
    case TsFmtMode::SHORT:
    {
        const auto [tm, ns] = localTime(_nsFromOrigin);
        auto time = fmt::format("{:%T}", tm);
        auto nsStr = fmt::format("{:09}", ns);
        auto utcOffset = fmtUtcOffset(tm);

        if (effFmtMode == TsFmtMode::LONG) {
            return LongFmtTs {
                fmt::format("{:%F}", tm), std::move(time), std::move(nsStr), std::move(utcOffset)
            };
        }

        return ShortFmtTs {std::move(time), std::move(nsStr), std::move(utcOffset)};
    }

    case TsFmtMode::NS_FROM_ORIGIN:
        return NsFromOriginFmtTs {fmt::to_string(_nsFromOrigin)};

    case TsFmtMode::CYCLES:
        return CyclesFmtTs {fmt::to_string(_cycles)};
    }

    std::abort();
}

std::optional<std::string> constUtcOffset(const Ts& first, const Ts& last)
{
    assert(first.hasUnixEpochOrigin());
    assert(last.hasUnixEpochOrigin());
    assert(first <= last);

    constexpr auto nsPerDay = 86'400 * nsPerSec;
    const auto days = (last - first).ns() / nsPerDay;

    // don't bother with a longer range: assume the offset changes
    if (days > 3'660) {
        return std::nullopt;
    }

    auto utcOffset = fmtUtcOffset(localTime(last.nsFromOrigin()).first);

    // UTC offset changes are months apart: checking once a day is enough
    for (unsigned long long day = 0; day <= days; ++day) {
        const auto ns = first.nsFromOrigin() + static_cast<long long>(day) * nsPerDay;

        if (fmtUtcOffset(localTime(ns).first) != utcOffset) {
            return std::nullopt;
        }
    }

    return utcOffset;
}

} // namespace jacques
