/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PADDING_PACKET_REGION_HPP
#define _JACQUES_PADDING_PACKET_REGION_HPP

#include "packet-region.hpp"

namespace jacques {

class PaddingPacketRegion :
    public PacketRegion
{
public:
    explicit PaddingPacketRegion(const PacketSegment& segment, Scope::SP scope);

private:
    void _accept(PacketRegionVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_PADDING_PACKET_REGION_HPP
