/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "padding-data-region.hpp"

namespace jacques {

PaddingDataRegion::PaddingDataRegion(const DataSegment& segment, Data&& data,
                                     Scope::SP scope,
                                     const boost::optional<ByteOrder>& byteOrder) :
    DataRegion {segment, std::move(data), std::move(scope), byteOrder}
{
}

} // namespace jacques
