/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data/duration.hpp"
#include "utils.hpp"

namespace jacques {

Duration::Duration(const unsigned long long ns) :
    _ns {ns}
{
}

Duration::Parts Duration::parts() const
{
    const auto nsPart = _ns % 1'000'000'000;
    const auto sDiff = (_ns - nsPart) / 1'000'000'000;
    const auto sPart = sDiff % 60;
    const auto mDiff = (sDiff - sPart) / 60;
    const auto mPart = mDiff % 60;
    const auto hPart = (mDiff - mPart) / 60;

    return {hPart, mPart, sPart, nsPart};
}

void Duration::format(char * const buf, const Size bufSize) const
{
    // TODO: use bufSize
    JACQUES_UNUSED(bufSize);

    auto bufAt = buf;
    const auto parts = this->parts();

    if (parts.hours > 0) {
        bufAt += std::sprintf(bufAt, "%llu:", parts.hours);
    }

    if (parts.minutes > 0 || parts.hours > 0) {
        const char *fmt;

        if (parts.hours > 0) {
            fmt = "%02llu:";
        } else {
            fmt = "%llu:";
        }

        bufAt += std::sprintf(bufAt, fmt, parts.minutes);
    }

    if (parts.seconds > 0 || parts.minutes > 0 || parts.hours > 0) {
        const char *fmt;

        if (parts.minutes > 0 || parts.hours > 0) {
            fmt = "%02llu";
        } else {
            fmt = "%llu";
        }

        bufAt += std::sprintf(bufAt, fmt, parts.seconds);
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
