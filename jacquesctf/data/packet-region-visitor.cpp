/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "packet-region-visitor.hpp"

namespace jacques {

PacketRegionVisitor::~PacketRegionVisitor()
{
}

void PacketRegionVisitor::visit(const ContentPacketRegion&)
{
}

void PacketRegionVisitor::visit(const PaddingPacketRegion&)
{
}

void PacketRegionVisitor::visit(const ErrorPacketRegion&)
{
}

} // namespace jacques
