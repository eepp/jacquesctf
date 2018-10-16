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
#include <boost/optional.hpp>

#include "data-region-visitor.hpp"
#include "data-segment.hpp"
#include "scope.hpp"

namespace jacques {

enum class ByteOrder
{
    BIG,
    LITTLE,
};

class DataRegion
{
public:
    using SP = std::shared_ptr<DataRegion>;

    /*
     * Bytes of this region (`begin` to `end`, excluded). The exact bit
     * where the data starts within the first byte is given by
     * segment().offsetInPacketExtraBits().
     *
     * The actual data is located within the memory mapped file owned by
     * the `Packet` object from which this data region was created. Thus
     * the packet object must exist for this range to be valid.
     */
    class DataRange
    {
    public:
        explicit DataRange(const std::uint8_t * const begin,
                           const std::uint8_t * const end) :
            _begin {begin},
            _end {end},
            _size {DataSize::fromBytes(end - begin)}
        {
        }

        const std::uint8_t *begin() const noexcept
        {
            return _begin;
        }

        const std::uint8_t *end() const noexcept
        {
            return _end;
        }

        const DataSize& size() const noexcept
        {
            return _size;
        }

    private:
        const std::uint8_t * const _begin;
        const std::uint8_t * const _end;
        const DataSize _size;
    };

protected:
    explicit DataRegion(const DataSegment& segment,
                        const DataRange& dataRange, Scope::SP scope,
                        const boost::optional<ByteOrder>& byteOrder = boost::none);

public:
    virtual ~DataRegion();

    void accept(DataRegionVisitor& visitor)
    {
        this->_accept(visitor);
    }

    const Scope *scope() const noexcept
    {
        return _scope.get();
    }

    Scope::SP scopePtr() const
    {
        return _scope;
    }

    const DataSegment& segment() const noexcept
    {
        return _segment;
    }

    const DataRange& dataRange() const noexcept
    {
        return _dataRange;
    }

    const boost::optional<ByteOrder>& byteOrder() const noexcept
    {
        return _byteOrder;
    }

    const boost::optional<Index>& previousDataRegionOffsetInPacketBits() const noexcept
    {
        return _prevDataRegionOffsetInPacketBits;
    }

    void previousDataRegionOffsetInPacketBits(const Index offsetInPacketBits)
    {
        _prevDataRegionOffsetInPacketBits = offsetInPacketBits;
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
    virtual void _accept(DataRegionVisitor& visitor) = 0;

private:
    boost::optional<Index> _prevDataRegionOffsetInPacketBits;
    DataSegment _segment;
    DataRange _dataRange;
    Scope::SP _scope;
    const boost::optional<ByteOrder> _byteOrder;
};

using DataRegions = std::vector<DataRegion::SP>;

} // namespace jacques

#endif // _JACQUES_DATA_REGION_HPP
