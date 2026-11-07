/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iterator>
#include <fmt/format.h>

#include "duration.hpp"

namespace jacques {

Duration::Duration(const unsigned long long ns) noexcept :
    _ns {ns}
{
}

Duration::Parts Duration::parts() const noexcept
{
    const auto nsPart = _ns % 1'000'000'000;
    const auto sDiff = (_ns - nsPart) / 1'000'000'000;
    const auto sPart = sDiff % 60;
    const auto mDiff = (sDiff - sPart) / 60;
    const auto mPart = mDiff % 60;
    const auto hPart = (mDiff - mPart) / 60;

    return {hPart, mPart, sPart, nsPart};
}

std::string Duration::format() const
{
    std::string str;
    const auto out = std::back_inserter(str);
    const auto parts = this->parts();

    if (parts.hours > 0) {
        fmt::format_to(out, "{}:", parts.hours);
    }

    if (parts.mins > 0 || parts.hours > 0) {
        fmt::format_to(out, "{:0{}}:", parts.mins, parts.hours > 0 ? 2 : 0);
    }

    if (parts.secs > 0 || parts.mins > 0 || parts.hours > 0) {
        fmt::format_to(out, "{:0{}}", parts.secs, parts.mins > 0 || parts.hours > 0 ? 2 : 0);
    }

    fmt::format_to(out, ".{:09}", parts.ns);
    return str;
}

} // namespace jacques
