/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_TS_HPP
#define _JACQUES_DATA_TS_HPP

#include <optional>
#include <string>
#include <variant>
#include <boost/operators.hpp>
#include <yactfr/yactfr.hpp>

namespace jacques {

enum class TsFmtMode
{
    LONG,
    SHORT,
    NS_FROM_ORIGIN,
    CYCLES,
};

// timestamp formatted with `TsFmtMode::LONG`
struct LongFmtTs final
{
    std::string date;
    std::string time;
    std::string ns;
    std::string utcOffset;
};

// timestamp formatted with `TsFmtMode::SHORT`
struct ShortFmtTs final
{
    std::string time;
    std::string ns;
    std::string utcOffset;
};

// timestamp formatted with `TsFmtMode::NS_FROM_ORIGIN`
struct NsFromOriginFmtTs final
{
    std::string ns;
};

// timestamp formatted with `TsFmtMode::CYCLES`
struct CyclesFmtTs final
{
    std::string cycles;
};

using FmtTs = std::variant<LongFmtTs, ShortFmtTs, NsFromOriginFmtTs, CyclesFmtTs>;

class Ts final :
    public boost::totally_ordered<Ts>
{
public:
    explicit Ts(unsigned long long cycles, const yactfr::ClockType& clkType) noexcept;

    explicit Ts(unsigned long long cycles, unsigned long long freq, long long offsetSecs,
                unsigned long long offsetCycles, bool hasUnixEpochOrigin) noexcept;

    unsigned long long cycles() const noexcept
    {
        return _cycles;
    }

    unsigned long long freq() const noexcept
    {
        return _freq;
    }

    long long nsFromOrigin() const noexcept
    {
        return _nsFromOrigin;
    }

    // true if the origin of the clock is the Unix epoch
    bool hasUnixEpochOrigin() const noexcept
    {
        return _hasUnixEpochOrigin;
    }

    /*
     * Format mode which format() actually uses for `fmtMode`.
     *
     * This is `TsFmtMode::NS_FROM_ORIGIN` instead of a date format mode
     * when the origin of the clock isn't the Unix epoch.
     */
    TsFmtMode effectiveFmtMode(TsFmtMode fmtMode) const noexcept;

    /*
     * Formats this timestamp with the effective format mode of
     * `fmtMode` (see effectiveFmtMode()).
     */
    FmtTs format(TsFmtMode fmtMode = TsFmtMode::LONG) const;

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
    bool _hasUnixEpochOrigin;
};

/*
 * Returns the local UTC offset, formatted like `LongFmtTs::utcOffset`,
 * if it's the same for all the times from `first` to `last`, or
 * `std::nullopt` otherwise.
 *
 * Both `first` and `last` must have a Unix epoch origin, and `first`
 * must not be greater than `last`.
 */
std::optional<std::string> constUtcOffset(const Ts& first, const Ts& last);

} // namespace jacques

#endif // _JACQUES_DATA_TS_HPP
