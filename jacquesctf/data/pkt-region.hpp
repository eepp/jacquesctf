/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_REGION_HPP
#define _JACQUES_DATA_PKT_REGION_HPP

#include <memory>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/core/noncopyable.hpp>

#include "pkt-region-visitor.hpp"
#include "pkt-segment.hpp"
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
 * An error region is a special region which is always the last one of
 * the packet if it exists. It's used to indicate that, starting from a
 * specific bit until the end of the packet, this region cannot be
 * decoded.
 *
 * A packet region doesn't contain the actual bits of data. A content
 * packet region contains the actual decoded value, but to get the
 * packet bits, you need to call Pkt::bitArray() to get a corresponding
 * bit array (of which the lifetime depends on the packet object which
 * created it).
 *
 * Packet regions are comparable as long as they're known to be part of
 * the same packet: only their segments are compared.
 */
class PktRegion :
    public boost::totally_ordered<PktRegion>,
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<PktRegion>;
    using SPC = std::shared_ptr<const PktRegion>;

protected:
    explicit PktRegion(const PktSegment& segment, Scope::SP scope = nullptr) noexcept;

public:
    virtual ~PktRegion() = default;

    void accept(PktRegionVisitor& visitor)
    {
        this->_accept(visitor);
    }

    const Scope *scope() const noexcept
    {
        return _scope.get();
    }

    Scope::SP scopePtr() noexcept
    {
        return _scope;
    }

    Scope::SPC scopePtr() const noexcept
    {
        return _scope;
    }

    const PktSegment& segment() const noexcept
    {
        return _seg;
    }

    const boost::optional<Index>& prevRegionOffsetInPktBits() const noexcept
    {
        return _prevRegionOffsetInPktBits;
    }

    void prevRegionOffsetInPktBits(const Index offsetInPktBits) noexcept
    {
        _prevRegionOffsetInPktBits = offsetInPktBits;
    }

    bool operator<(const PktRegion& other) const noexcept
    {
        return _seg < other._seg;
    }

    bool operator==(const PktRegion& other) const noexcept
    {
        return _seg == other._seg;
    }

protected:
    PktSegment& _segment() noexcept
    {
        return _seg;
    }

private:
    virtual void _accept(PktRegionVisitor& visitor) = 0;

private:
    boost::optional<Index> _prevRegionOffsetInPktBits;
    PktSegment _seg;
    Scope::SP _scope;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_REGION_HPP
