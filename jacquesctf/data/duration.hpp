/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_DURATION_HPP
#define _JACQUES_DATA_DURATION_HPP

#include <string>
#include <string_view>
#include <boost/operators.hpp>
#include <fmt/format.h>

namespace jacques {

class Duration final :
    public boost::totally_ordered<Duration>
{
public:
    struct Parts final
    {
        unsigned long long hours;
        unsigned long long mins;
        unsigned long long secs;
        unsigned long long ns;
    };

public:
    constexpr explicit Duration(const unsigned long long ns) noexcept :
        _ns {ns}
    {
    }

    constexpr unsigned long long ns() const noexcept
    {
        return _ns;
    }

    constexpr Parts parts() const noexcept
    {
        const auto secs = _ns / 1'000'000'000;
        const auto mins = secs / 60;

        return {mins / 60, mins % 60, secs % 60, _ns % 1'000'000'000};
    }

    std::string format() const;

    constexpr bool operator==(const Duration& other) const noexcept
    {
        return _ns == other._ns;
    }

    constexpr bool operator<(const Duration& other) const noexcept
    {
        return _ns < other._ns;
    }

private:
    unsigned long long _ns;
};

} // namespace jacques

template <>
struct fmt::formatter<jacques::Duration> : fmt::formatter<std::string_view>
{
    auto format(const jacques::Duration& duration, fmt::format_context& ctx) const
    {
        return fmt::formatter<std::string_view>::format(duration.format(), ctx);
    }
};

#endif // _JACQUES_DATA_DURATION_HPP
