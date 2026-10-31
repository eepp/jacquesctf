/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "data-len.hpp"
#include "utils.hpp"

namespace jacques {

std::pair<std::string, std::string> DataLen::format(const utils::LenFmtMode fmtMode,
                                                    const std::optional<char>& sep) const
{
    return utils::formatLen(_lenBits, fmtMode, sep);
}

} // namespace jacques
