/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "duration.hpp"
#include "utils.hpp"

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

// TODO: use the second parameter
void Duration::format(char * const buf, const Size) const
{
    auto bufAt = buf;
    const auto parts = this->parts();

    if (parts.hours > 0) {
        bufAt += std::sprintf(bufAt, "%llu:", parts.hours);
    }

    if (parts.mins > 0 || parts.hours > 0) {
        bufAt += std::sprintf(bufAt, (parts.hours > 0) ? "%02llu:" : "%llu:", parts.mins);
    }

    if (parts.secs > 0 || parts.mins > 0 || parts.hours > 0) {
        bufAt += std::sprintf(bufAt, (parts.mins > 0 || parts.hours > 0) ? "%02llu" : "%llu",
                              parts.secs);
    }

    std::sprintf(bufAt, ".%09llu", parts.ns);
}

std::string Duration::format() const
{
    std::array<char, 64> buf;

    this->format(buf.data(), buf.size());
    return buf.data();
}

} // namespace jacques
