/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PACKET_CHECKPOINTS_BUILD_LISTENER_HPP
#define _JACQUES_DATA_PACKET_CHECKPOINTS_BUILD_LISTENER_HPP

#include <memory>
#include <yactfr/metadata/fwd.hpp>

#include "event-record.hpp"
#include "packet-index-entry.hpp"

namespace jacques {

class PacketCheckpointsBuildListener
{
protected:
    PacketCheckpointsBuildListener();

public:
    virtual ~PacketCheckpointsBuildListener();

    void startBuild(const PacketIndexEntry& packetIndexEntry)
    {
        this->_startBuild(packetIndexEntry);
    }

    void update(const EventRecord& eventRecord)
    {
        this->_update(eventRecord);
    }

    void endBuild()
    {
        this->_endBuild();
    }

protected:
    virtual void _startBuild(const PacketIndexEntry& packetIndexEntry);
    virtual void _update(const EventRecord& eventRecord);
    virtual void _endBuild();
};

} // namespace jacques

#endif // _JACQUES_DATA_PACKET_CHECKPOINTS_BUILD_LISTENER_HPP
