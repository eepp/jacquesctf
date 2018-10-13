/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-region.hpp"

namespace jacques {

DataRegion::DataRegion(const DataSegment& segment, Data&& data,
                       const boost::optional<ByteOrder>& byteOrder) :
    _segment {segment},
    _data {std::move(data)},
    _byteOrder {byteOrder}
{
}

} // namespace jacques
