/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data/padding-packet-region.hpp"

namespace jacques {

PaddingPacketRegion::PaddingPacketRegion(const PacketSegment& segment,
                                         Scope::SP scope) :
    PacketRegion {segment, std::move(scope)}
{
}

void PaddingPacketRegion::_accept(PacketRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
