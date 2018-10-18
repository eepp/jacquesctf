/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-size.hpp"
#include "utils.hpp"

namespace jacques {

DataSize DataSize::fromBytes(Size sizeBytes)
{
    return {sizeBytes * 8};
}

DataSize::DataSize(const Size sizeBits) :
    _sizeBits {sizeBits}
{
}

std::pair<std::string, std::string> DataSize::format(const utils::SizeFormatMode formatMode,
                                                     const boost::optional<char>& sep) const
{
    return utils::formatSize(_sizeBits, formatMode, sep);
}

} // namespace jacques
