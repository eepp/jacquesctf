/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_CONTENT_PACKET_REGION_HPP
#define _JACQUES_DATA_CONTENT_PACKET_REGION_HPP

#include <memory>
#include <cstdint>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "data/packet-region.hpp"
#include "data/scope.hpp"

namespace jacques {

class ContentPacketRegion :
    public PacketRegion
{
public:
    using Value = boost::variant<std::int64_t,
                                 std::uint64_t,
                                 double,
                                 std::string>;

public:
    explicit ContentPacketRegion(const PacketSegment& segment, Scope::SP scope,
                                 const yactfr::DataType& dataType,
                                 const boost::optional<Value>& value);

    const yactfr::DataType& dataType() const noexcept
    {
        return *_dataType;
    }

    const boost::optional<Value>& value() const noexcept
    {
        return _value;
    }

private:
    void _accept(PacketRegionVisitor& visitor) override;

private:
    const yactfr::DataType *_dataType;
    boost::optional<Value> _value;
};

} // namespace jacques

#endif // _JACQUES_DATA_CONTENT_PACKET_REGION_HPP
