/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_PACKET_STATE_HPP
#define _JACQUES_INSPECT_COMMAND_PACKET_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "data/packet.hpp"
#include "aliases.hpp"

namespace jacques {

class State;

class PacketState
{
public:
    explicit PacketState(State& state, const Metadata& metadata,
                         Packet& packet);
    void gotoPreviousEventRecord(Size count = 1);
    void gotoNextEventRecord(Size count = 1);
    void gotoPreviousPacketRegion();
    void gotoNextPacketRegion();
    void gotoPacketContext();
    void gotoLastPacketRegion();
    void gotoPacketRegionNextParent();
    void gotoPacketRegionAtOffsetInPacketBits(Index offsetBits);

    void gotoPacketRegionAtOffsetInPacketBits(const PacketRegion& region)
    {
        this->gotoPacketRegionAtOffsetInPacketBits(region.segment().offsetInPacketBits());
    }

    Packet& packet() noexcept
    {
        return *_packet;
    }

    const PacketIndexEntry& packetIndexEntry() const noexcept
    {
        return _packet->indexEntry();
    }

    Index curOffsetInPacketBits() const noexcept
    {
        return _curOffsetInPacketBits;
    }

    const EventRecord *currentEventRecord()
    {
        const auto& packetRegion = _packet->regionAtOffsetInPacketBits(_curOffsetInPacketBits);
        const auto& scope = packetRegion.scope();

        if (!scope) {
            return nullptr;
        }

        // can return `nullptr`
        return scope->eventRecord();
    }

    const PacketRegion& currentPacketRegion()
    {
        return _packet->regionAtOffsetInPacketBits(_curOffsetInPacketBits);
    }

    State& state() noexcept
    {
        return *_state;
    }

    const State& state() const noexcept
    {
        return *_state;
    }

private:
    State * const _state;
    const Metadata * const _metadata;
    Packet * const _packet;
    Index _curOffsetInPacketBits = 0;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_PACKET_STATE_HPP
