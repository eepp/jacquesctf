/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "error-packet-region.hpp"

namespace jacques {

ErrorPacketRegion::ErrorPacketRegion(const PacketSegment& segment) :
    PacketRegion {segment}
{
}

void ErrorPacketRegion::_accept(PacketRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
