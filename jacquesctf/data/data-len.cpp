/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-len.hpp"
#include "utils.hpp"

namespace jacques {

std::pair<std::string, std::string> DataLen::format(const utils::LenFmtMode fmtMode,
                                                    const boost::optional<char>& sep) const
{
    return utils::formatLen(_lenBits, fmtMode, sep);
}

} // namespace jacques
