/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_ERROR_DATA_REGION_HPP
#define _JACQUES_ERROR_DATA_REGION_HPP

#include <memory>

#include "data-region.hpp"

namespace jacques {

class ErrorDataRegion :
    public DataRegion
{
public:
    explicit ErrorDataRegion(const DataSegment& segment,
                             const DataRange& dataRange,
                             const boost::optional<ByteOrder>& byteOrder);

private:
    void _accept(DataRegionVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_ERROR_DATA_REGION_HPP
