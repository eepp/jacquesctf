/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_SEGMENT_HPP
#define _JACQUES_DATA_PKT_SEGMENT_HPP

#include <cstdint>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"
#include "data-len.hpp"
#include "bo.hpp"
#include "bit-loc.hpp"

namespace jacques {

/*
 * A packet segment is a section within a packet: it is specified by an
 * offset within the packet, a length, and a byte order.
 *
 * You can know exactly which bits within the packet belong to a given
 * packet segment. However, a packet segment does not contain the actual
 * bits of data. To get the packet bits, you need to call
 * Pkt::bitArray() to get a corresponding bit array (of which the
 * lifetime depends on the packet object which created it).
 *
 * Packet segments are comparable as long as they are known to be part
 * of the same packet and disjoint: only their offsets are compared.
 */
class PktSegment final :
    public boost::totally_ordered<PktSegment>
{
public:
    explicit PktSegment() noexcept = default;
    explicit PktSegment(Index offsetInPktBits, const DataLen& len, OptBo bo = boost::none) noexcept;
    PktSegment(const PktSegment&) noexcept = default;
    PktSegment& operator=(const PktSegment&) noexcept = default;

    Index offsetInPktBits() const noexcept
    {
        return _offsetInPktBits;
    }

    boost::optional<Index> endOffsetInPktBits() const noexcept
    {
        if (!_len) {
            return boost::none;
        }

        return _offsetInPktBits + _len->bits();
    }

    void offsetInPktBits(const Index offsetInPktBits) noexcept
    {
        _offsetInPktBits = offsetInPktBits;
    }

    Index offsetInFirstByteBits() const noexcept
    {
        return _offsetInPktBits & 7;
    }

    const boost::optional<DataLen>& len() const noexcept
    {
        return _len;
    }

    void len(const DataLen& len) noexcept
    {
        _len = len;
    }

    const OptBo& bo() const noexcept
    {
        return _bo;
    }

    void bo(const OptBo& bo) noexcept
    {
        _bo = bo;
    }

    bool operator<(const PktSegment& other) const noexcept
    {
        return _offsetInPktBits < other._offsetInPktBits;
    }

    bool operator==(const PktSegment& other) const noexcept
    {
        return _offsetInPktBits == other._offsetInPktBits;
    }

private:
    Index _offsetInPktBits = 0;
    boost::optional<DataLen> _len;
    OptBo _bo;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_SEGMENT_HPP
