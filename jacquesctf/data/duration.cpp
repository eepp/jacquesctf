/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <fmt/format.h>

#include "duration.hpp"

namespace jacques {

std::string Duration::format() const
{
    const auto parts = this->parts();

    if (parts.hours > 0) {
        return fmt::format("{}:{:02}:{:02}.{:09}", parts.hours, parts.mins, parts.secs, parts.ns);
    }

    if (parts.mins > 0) {
        return fmt::format("{}:{:02}.{:09}", parts.mins, parts.secs, parts.ns);
    }

    if (parts.secs > 0) {
        return fmt::format("{}.{:09}", parts.secs, parts.ns);
    }

    return fmt::format(".{:09}", parts.ns);
}

} // namespace jacques
