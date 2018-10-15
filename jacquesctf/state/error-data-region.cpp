/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "error-data-region.hpp"

namespace jacques {

ErrorDataRegion::ErrorDataRegion(const DataSegment& segment,
                                 const DataRange& dataRange,
                                 const boost::optional<ByteOrder>& byteOrder) :
    DataRegion {segment, dataRange, nullptr, byteOrder}
{
}

} // namespace jacques
