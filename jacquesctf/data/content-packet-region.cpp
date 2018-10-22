/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "content-packet-region.hpp"

namespace jacques {

static OptByteOrder byteOrderFromDataType(const yactfr::DataType& dataType)
{
    if (dataType.isBitArrayType()) {
        switch (dataType.asBitArrayType()->byteOrder()) {
        case yactfr::ByteOrder::BIG:
            return ByteOrder::BIG;

        case yactfr::ByteOrder::LITTLE:
            return ByteOrder::LITTLE;
        }
    }

    return boost::none;
}

ContentPacketRegion::ContentPacketRegion(const PacketSegment& segment,
                                         Scope::SP scope,
                                         const yactfr::DataType& dataType,
                                         const boost::optional<Value>& value) :
    PacketRegion {
        segment,
        std::move(scope)
    },
    _dataType {&dataType},
    _value {value}
{
    this->_segment().byteOrder(byteOrderFromDataType(dataType));
}

void ContentPacketRegion::_accept(PacketRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
