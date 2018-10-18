/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-region.hpp"

namespace jacques {

DataRegion::DataRegion(const DataSegment& segment, const DataRange& dataRange,
                       Scope::SP scope,
                       const boost::optional<ByteOrder>& byteOrder) :
    _segment {segment},
    _dataRange {dataRange},
    _scope {std::move(scope)},
    _byteOrder {byteOrder}
{
}

DataRegion::~DataRegion()
{
}

} // namespace jacques