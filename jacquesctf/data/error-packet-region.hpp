/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_ERROR_PACKET_REGION_HPP
#define _JACQUES_DATA_ERROR_PACKET_REGION_HPP

#include <memory>

#include "packet-region.hpp"

namespace jacques {

class ErrorPacketRegion :
    public PacketRegion
{
public:
    explicit ErrorPacketRegion(const PacketSegment& segment);

private:
    void _accept(PacketRegionVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_DATA_ERROR_PACKET_REGION_HPP
