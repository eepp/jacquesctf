/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_REGION_HPP
#define _JACQUES_DATA_REGION_HPP

#include <memory>
#include <vector>
#include <boost/operators.hpp>

#include "data-segment.hpp"

namespace jacques {

enum class ByteOrder
{
    BIG,
    LITTLE,
    USELESS,
};

class DataRegion
{
public:
    using SP = std::shared_ptr<DataRegion>;

    /*
     * Bytes of this region. It can contain more bytes than the actual
     * region described by the region's segment. The exact bit where the
     * data starts within the first byte is given by
     * DataSegment::offsetInPacketExtraBits() for this region's segment.
     */
    using Data = std::vector<std::uint8_t>;

protected:
    explicit DataRegion(const DataSegment& segment,
                        Data&& data, ByteOrder byteOrder);

public:
    const DataSegment& segment() const noexcept
    {
        return _segment;
    }

    const Data& data() const noexcept
    {
        return _data;
    }

    ByteOrder byteOrder() const noexcept
    {
        return _byteOrder;
    }

    bool operator<(const DataRegion& other)
    {
        return _segment < other._segment;
    }

    bool operator==(const DataRegion& other)
    {
        return _segment == other._segment;
    }

private:
    DataSegment _segment;
    Data _data;
    ByteOrder _byteOrder;
};

} // namespace jacques

#endif // _JACQUES_DATA_REGION_HPP
