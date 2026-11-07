/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cstdlib>
#include <fmt/format.h>

#include "data-len.hpp"
#include "utils.hpp"

namespace jacques {

FmtDataLen DataLen::format(const utils::LenFmtMode fmtMode, const std::optional<char>& sep) const
{
    const auto lenBytes = this->bytes();
    const auto extraBitsStr = [this, fmtMode]() -> std::optional<std::string> {
        if (this->hasExtraBits() && (fmtMode == utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS ||
                                     fmtMode == utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS)) {
            return fmt::to_string(this->extraBits());
        }

        return std::nullopt;
    };

    const auto fmtNum = [&sep](const Size val) {
        return sep ? utils::sepNumber(static_cast<long long>(val), *sep) : fmt::to_string(val);
    };

    switch (fmtMode) {
    case utils::LenFmtMode::FULL_FLOOR:
    case utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS:
    {
        const char *unit = "B";
        auto val = static_cast<double>(lenBytes);

        if (lenBytes >= 1024 * 1024 * 1024) {
            val /= 1024. * 1024 * 1024;
            unit = "GiB";
        } else if (lenBytes >= 1024 * 1024) {
            val /= 1024. * 1024;
            unit = "MiB";
        } else if (lenBytes >= 1024) {
            val /= 1024.;
            unit = "KiB";
        }

        return FullFmtDataLen {fmt::format("{:.3f}", val), unit, extraBitsStr()};
    }

    case utils::LenFmtMode::BYTES_FLOOR:
    case utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS:
        return BytesFmtDataLen {fmtNum(lenBytes), extraBitsStr()};

    case utils::LenFmtMode::BITS:
        return BitsFmtDataLen {fmtNum(_lenBits)};
    }

    std::abort();
}

} // namespace jacques
