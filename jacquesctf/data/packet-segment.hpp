/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PACKET_SEGMENT_HPP
#define _JACQUES_DATA_PACKET_SEGMENT_HPP

#include <cstdint>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"
#include "data/data-size.hpp"
#include "byte-order.hpp"
#include "bit-location.hpp"

namespace jacques {

/*
 * A packet segment is a section within a packet: it is specified by an
 * offset within the packet, a size, and a byte order.
 *
 * You can know exactly which bits within the packet belong to a given
 * packet segment. However, a packet segment does not contain the actual
 * bits of data. To get the packet bits, you need to call
 * Packet::bitArray() to get a corresponding bit array (of which the
 * lifetime depends on the packet object which created it).
 *
 * Packet segments are comparable as long as they are known to be part
 * of the same packet and disjoint: only their offsets are compared.
 */
class PacketSegment :
    public boost::totally_ordered<PacketSegment>
{
public:
    PacketSegment() = default;
    PacketSegment(const PacketSegment&) = default;
    explicit PacketSegment(Index offsetInPacketBits, const DataSize& size,
                           const OptByteOrder& byteOrder = boost::none);
    PacketSegment& operator=(const PacketSegment&) = default;

    Index offsetInPacketBits() const noexcept
    {
        return _offsetInPacketBits;
    }

    boost::optional<Index> endOffsetInPacketBits() const noexcept
    {
        if (!_size) {
            return boost::none;
        }

        return _offsetInPacketBits + _size->bits();
    }

    void offsetInPacketBits(const Index offsetInPacketBits) noexcept
    {
        _offsetInPacketBits = offsetInPacketBits;
    }

    Index offsetInFirstByteBits() const noexcept
    {
        return _offsetInPacketBits & 7;
    }

    const boost::optional<DataSize>& size() const noexcept
    {
        return _size;
    }

    void size(const DataSize& size) noexcept
    {
        _size = size;
    }

    const OptByteOrder& byteOrder() const noexcept
    {
        return _byteOrder;
    }

    void byteOrder(const OptByteOrder& byteOrder) noexcept
    {
        _byteOrder = byteOrder;
    }

    bool operator<(const PacketSegment& other) const noexcept
    {
        return _offsetInPacketBits < other._offsetInPacketBits;
    }

    bool operator==(const PacketSegment& other) const noexcept
    {
        return _offsetInPacketBits == other._offsetInPacketBits;
    }

private:
    Index _offsetInPacketBits = 0;
    boost::optional<DataSize> _size;
    OptByteOrder _byteOrder;
};

} // namespace jacques

#endif // _JACQUES_DATA_PACKET_SEGMENT_HPP
