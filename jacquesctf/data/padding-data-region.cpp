/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "padding-data-region.hpp"

namespace jacques {

PaddingDataRegion::PaddingDataRegion(const DataSegment& segment,
                                     const DataRange& dataRange,
                                     Scope::SP scope,
                                     const boost::optional<ByteOrder>& byteOrder) :
    DataRegion {segment, dataRange, std::move(scope), byteOrder}
{
}

void PaddingDataRegion::_accept(DataRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
