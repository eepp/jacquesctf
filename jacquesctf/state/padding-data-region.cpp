/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "padding-data-region.hpp"

namespace jacques {

PaddingDataRegion::PaddingDataRegion(const DataSegment& segment, Data&& data,
                                     const ByteOrder byteOrder) :
    DataRegion {segment, std::move(data), byteOrder}
{
}

} // namespace jacques
