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

#include "aliases.hpp"

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
    explicit Duration() noexcept = default;
    explicit Duration(unsigned long long ns) noexcept;
    Duration(const Duration&) noexcept = default;
    Duration& operator=(const Duration&) noexcept = default;
    Parts parts() const noexcept;
    std::string format() const;

    unsigned long long ns() const noexcept
    {
        return _ns;
    }

    bool operator==(const Duration& other) const noexcept
    {
        return _ns == other._ns;
    }

    bool operator<(const Duration& other) const noexcept
    {
        return _ns < other._ns;
    }

private:
    unsigned long long _ns = 0;
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
