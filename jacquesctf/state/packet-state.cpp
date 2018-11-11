/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "packet-state.hpp"
#include "packet-region.hpp"
#include "state.hpp"
#include "message.hpp"

namespace jacques {

PacketState::PacketState(State& state, Packet& packet) :
    _state {&state},
    _packet {&packet}
{
}

void PacketState::gotoPreviousEventRecord(Size count)
{
    if (_packet->eventRecordCount() == 0) {
        return;
    }

    const auto curEventRecord = this->currentEventRecord();

    if (!curEventRecord) {
        if (_curOffsetInPacketBits >=
                _packet->indexEntry().effectiveContentSize()) {
            auto lastEr = _packet->lastEventRecord();

            assert(lastEr);
            this->gotoPacketRegionAtOffsetInPacketBits(lastEr->segment().offsetInPacketBits());
        }

        return;
    }

    if (curEventRecord->indexInPacket() == 0) {
        return;
    }

    count = std::min(curEventRecord->indexInPacket(), count);

    const auto& prevEventRecord = _packet->eventRecordAtIndexInPacket(curEventRecord->indexInPacket() - count);

    this->gotoPacketRegionAtOffsetInPacketBits(prevEventRecord.segment().offsetInPacketBits());
}

void PacketState::gotoNextEventRecord(Size count)
{
    if (_packet->eventRecordCount() == 0) {
        return;
    }

    const auto curEventRecord = this->currentEventRecord();
    Index newIndex = 0;

    if (curEventRecord) {
        count = std::min(_packet->eventRecordCount() -
                         curEventRecord->indexInPacket(), count);
        newIndex = curEventRecord->indexInPacket() + count;
    }

    if (newIndex >= _packet->eventRecordCount()) {
        return;
    }

    const auto& nextEventRecord = _packet->eventRecordAtIndexInPacket(newIndex);

    if (nextEventRecord.segment().offsetInPacketBits())

    this->gotoPacketRegionAtOffsetInPacketBits(nextEventRecord.segment().offsetInPacketBits());
}

void PacketState::gotoPreviousPacketRegion()
{
    if (!_packet->hasData()) {
        return;
    }

    if (_curOffsetInPacketBits == 0) {
        return;
    }

    const auto& currentPacketRegion = this->currentPacketRegion();

    if (currentPacketRegion.previousRegionOffsetInPacketBits()) {
        this->gotoPacketRegionAtOffsetInPacketBits(*currentPacketRegion.previousRegionOffsetInPacketBits());
        return;
    }

    const auto& prevPacketRegion = _packet->regionAtOffsetInPacketBits(_curOffsetInPacketBits - 1);

    this->gotoPacketRegionAtOffsetInPacketBits(prevPacketRegion);
}

void PacketState::gotoNextPacketRegion()
{
    if (!_packet->hasData()) {
        return;
    }

    const auto& currentPacketRegion = this->currentPacketRegion();

    if (*currentPacketRegion.segment().endOffsetInPacketBits() ==
            _packet->indexEntry().effectiveTotalSize()) {
        return;
    }

    this->gotoPacketRegionAtOffsetInPacketBits(*currentPacketRegion.segment().endOffsetInPacketBits());
}

void PacketState::gotoPacketContext()
{
    const auto& offset = _packet->indexEntry().packetContextOffsetInPacketBits();

    if (!offset) {
        return;
    }

    this->gotoPacketRegionAtOffsetInPacketBits(*offset);
}

void PacketState::gotoLastPacketRegion()
{
    this->gotoPacketRegionAtOffsetInPacketBits(_packet->lastRegion());
}

void PacketState::gotoPacketRegionAtOffsetInPacketBits(const Index offsetInPacketBits)
{
    if (offsetInPacketBits == _curOffsetInPacketBits) {
        return;
    }

    if (offsetInPacketBits >= _packet->indexEntry().effectiveTotalSize()) {
        /*
         * This is a general protection against going too far. This can
         * happen, for example, if an event record begins exactly where
         * the packet content ends, which can happen if yactfr emitted a
         * "event record beginning" element immediately after aligning
         * its cursor.
         *
         * For example, given a packet content of 256 bits, if the
         * current cursor is at 224 bits, and the next event record
         * needs to be aligned to 64 bits, then yactfr aligns its cursor
         * to 256 bits, emits a "event record beginning" element at
         * 256 bits, and then eventually returns a decoding error
         * because it cannot decode more. In this case, Jacques CTF
         * considers there's an event record at offset 256, but there's
         * no packet region at offset 256.
         */
        return;
    }

    assert(offsetInPacketBits < _packet->indexEntry().effectiveTotalSize());
    _curOffsetInPacketBits = offsetInPacketBits;
    _state->_notify(Message::CUR_OFFSET_IN_PACKET_CHANGED);
}


} // namespace jacques
