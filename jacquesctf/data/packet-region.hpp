/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_REGION_HPP
#define _JACQUES_PACKET_REGION_HPP

#include <memory>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/core/noncopyable.hpp>

#include "packet-region-visitor.hpp"
#include "packet-segment.hpp"
#include "scope.hpp"

namespace jacques {

/*
 * A packet region is a region, within a packet, of content (trace
 * data), padding, or error.
 *
 * Content and padding regions can be linked to a scope, which itself
 * can be linked to an event record, so that you know in which scope and
 * event record a given region is.
 *
 * An error region is a special region which is always the packet's last
 * one if it exists. It is used to indicate that, starting from a
 * specific bit until the end of the packet, this region cannot be
 * decoded.
 *
 * A packet region does not contain the actual bits of data. A content
 * packet region contains the actual decoded value, but to get the
 * packet bits, you need to call Packet::bitArray() to get a
 * corresponding bit array (of which the lifetime depends on the packet
 * object which created it).
 *
 * Packet regions are comparable as long as they are known to be part of
 * the same packet: only their segments are compared.
 */
class PacketRegion :
    public boost::totally_ordered<PacketRegion>,
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<PacketRegion>;
    using SPC = std::shared_ptr<const PacketRegion>;

protected:
    explicit PacketRegion(const PacketSegment& segment, Scope::SP scope = nullptr);

public:
    virtual ~PacketRegion();

    void accept(PacketRegionVisitor& visitor)
    {
        this->_accept(visitor);
    }

    const Scope *scope() const noexcept
    {
        return _scope.get();
    }

    Scope::SP scopePtr()
    {
        return _scope;
    }

    Scope::SPC scopePtr() const
    {
        return _scope;
    }

    const PacketSegment& segment() const noexcept
    {
        return _seg;
    }

    const boost::optional<Index>& previousPacketRegionOffsetInPacketBits() const noexcept
    {
        return _prevPacketRegionOffsetInPacketBits;
    }

    void previousPacketRegionOffsetInPacketBits(const Index offsetInPacketBits)
    {
        _prevPacketRegionOffsetInPacketBits = offsetInPacketBits;
    }

    bool operator<(const PacketRegion& other) const noexcept
    {
        return _seg < other._seg;
    }

    bool operator==(const PacketRegion& other) const noexcept
    {
        return _seg == other._seg;
    }

protected:
    PacketSegment& _segment() noexcept
    {
        return _seg;
    }

private:
    virtual void _accept(PacketRegionVisitor& visitor) = 0;

private:
    boost::optional<Index> _prevPacketRegionOffsetInPacketBits;
    PacketSegment _seg;
    Scope::SP _scope;
};

} // namespace jacques

#endif // _JACQUES_PACKET_REGION_HPP
