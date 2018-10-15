/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PADDING_DATA_REGION_HPP
#define _JACQUES_PADDING_DATA_REGION_HPP

#include <memory>

#include "data-region.hpp"

namespace jacques {

class PaddingDataRegion :
    public DataRegion
{
public:
    explicit PaddingDataRegion(const DataSegment& segment,
                               const DataRange& dataRange, Scope::SP scope,
                               const boost::optional<ByteOrder>& byteOrder);
};

} // namespace jacques

#endif // _JACQUES_PADDING_DATA_REGION_HPP
