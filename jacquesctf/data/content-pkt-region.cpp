/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "content-pkt-region.hpp"

namespace jacques {

static OptBo boFromDt(const yactfr::DataType& dt)
{
    if (dt.isFixedLengthBitArrayType()) {
        return dt.asFixedLengthBitArrayType().byteOrder();
    }

    return boost::none;
}

ContentPktRegion::ContentPktRegion(const PktSegment& segment, Scope::SP scope,
                                   const yactfr::DataType& dt, boost::optional<Val> val) noexcept :
    PktRegion {
        segment,
        std::move(scope)
    },
    _dt {&dt},
    _val {std::move(val)}
{
    this->_segment().bo(boFromDt(dt));
}

void ContentPktRegion::_accept(PktRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
