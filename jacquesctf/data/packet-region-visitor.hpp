/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PACKET_REGION_VISITOR_HPP
#define _JACQUES_DATA_PACKET_REGION_VISITOR_HPP

namespace jacques {

class ContentPacketRegion;
class PaddingPacketRegion;
class ErrorPacketRegion;

class PacketRegionVisitor
{
public:
    virtual ~PacketRegionVisitor();
    virtual void visit(const ContentPacketRegion&);
    virtual void visit(const PaddingPacketRegion&);
    virtual void visit(const ErrorPacketRegion&);
};

} // namespace jacques

#endif // _JACQUES_DATA_PACKET_REGION_VISITOR_HPP
