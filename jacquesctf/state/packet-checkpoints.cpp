/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "packet-checkpoints.hpp"

namespace jacques {

PacketDecodingError::PacketDecodingError(const yactfr::DecodingError& decodingError,
                                         const PacketIndexEntry& packetIndexEntry) :
    _decodingError {decodingError},
    _packetIndexEntry {&packetIndexEntry}
{
}

PacketCheckpoints::PacketCheckpoints(yactfr::PacketSequence& seq,
                                     const Metadata& metadata,
                                     const PacketIndexEntry& packetIndexEntry,
                                     const Size step,
                                     PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
    this->_tryCreateCheckpoints(seq, metadata, packetIndexEntry,
                                step, packetCheckpointsBuildListener);
}

void PacketCheckpoints::_tryCreateCheckpoints(yactfr::PacketSequence& seq,
                                              const Metadata& metadata,
                                              const PacketIndexEntry& packetIndexEntry,
                                              const Size step,
                                              PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
    // we consider other errors (e.g., I/O) unrecoverable: do not catch them
    try {
        this->_createCheckpoints(seq, metadata, packetIndexEntry,
                                 step, packetCheckpointsBuildListener);
    } catch (const yactfr::DecodingError& ex) {
        _error = PacketDecodingError {ex, packetIndexEntry};
    }
}

void PacketCheckpoints::_createCheckpoints(yactfr::PacketSequence& seq,
                                           const Metadata& metadata,
                                           const PacketIndexEntry& packetIndexEntry,
                                           const Size step,
                                           PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
    const auto endIt = std::end(seq);
    auto it = seq.at(packetIndexEntry.offsetInDataStreamBytes());
    Index indexInPacket = 0;

    // create all checkpoints except (possibly) the last one
    while (it->kind() != yactfr::Element::Kind::PACKET_END) {
        if (it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            const auto curIndexInPacket = indexInPacket;

            ++indexInPacket;

            if (curIndexInPacket % step == 0) {
                this->_createCheckpoint(it, metadata,
                                        packetIndexEntry,
                                        curIndexInPacket,
                                        packetCheckpointsBuildListener);
                continue;
            }
        }

        ++it;
    }

    // find last event record and create a checkpoint if not already done
    if (_checkpoints.empty()) {
        // no event records in this packet!
        return;
    }

    yactfr::PacketSequenceIteratorPosition lastEventRecordPos;
    Index lastEventRecordIndex = 0;

    it.restorePosition(_checkpoints.back().second);
    indexInPacket = _checkpoints.back().first->indexInPacket();

    while (it->kind() != yactfr::Element::Kind::PACKET_END) {
        if (it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            it.savePosition(lastEventRecordPos);
            lastEventRecordIndex = indexInPacket;
            ++indexInPacket;
        }

        ++it;
    }

    assert(lastEventRecordPos);

    if (lastEventRecordPos == _checkpoints.back().second) {
        // checkpoint already exists
        return;
    }

    it.restorePosition(lastEventRecordPos);
    this->_createCheckpoint(it, metadata, packetIndexEntry,
                            lastEventRecordIndex,
                            packetCheckpointsBuildListener);
}

void PacketCheckpoints::_createCheckpoint(yactfr::PacketSequenceIterator& it,
                                          const Metadata& metadata,
                                          const PacketIndexEntry& packetIndexEntry,
                                          const Index indexInPacket,
                                          PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
    yactfr::PacketSequenceIteratorPosition pos;

    assert(it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING);
    it.savePosition(pos);

    auto eventRecord = EventRecord::createFromPacketSequenceIterator(it,
                                                                     metadata,
                                                                     packetIndexEntry.offsetInDataStreamBytes(),
                                                                     indexInPacket);
    _checkpoints.push_back({eventRecord, std::move(pos)});
    packetCheckpointsBuildListener.update(*eventRecord);
}

static bool indexInPacketLessThan(const PacketCheckpoints::Checkpoint& checkpoint,
                                  const Index indexInPacket)
{
    return checkpoint.first->indexInPacket() < indexInPacket;
}

static bool indexInPacketEqual(const EventRecord& eventRecord,
                               const Index indexInPacket)
{
    return eventRecord.indexInPacket() == indexInPacket;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtIndex(const Index indexInPacket) const
{
    return this->_nearestCheckpointBeforeOrAt(indexInPacket,
                                              indexInPacketLessThan,
                                              indexInPacketEqual);
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeIndex(const Index indexInPacket) const
{
    return this->_nearestCheckpointBefore(indexInPacket,
                                          indexInPacketLessThan);
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterIndex(const Index indexInPacket) const
{
    return this->_nearestCheckpointAfter(indexInPacket,
                                         [](const auto indexInPacket,
                                            const auto& checkpoint) {
        return indexInPacket < checkpoint.first->indexInPacket();
    });
}

static bool offsetInPacketBitsLessThan(const PacketCheckpoints::Checkpoint& checkpoint,
                                       const Index offsetInPacketBits)
{
    return checkpoint.first->segment().offsetInPacketBits() < offsetInPacketBits;
}

static bool offsetInPacketBitsEqual(const EventRecord& eventRecord,
                                    const Index offsetInPacketBits)
{
    return eventRecord.segment().offsetInPacketBits() == offsetInPacketBits;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtOffsetInPacketBits(const Index offsetInPacketBits) const
{
    return this->_nearestCheckpointBeforeOrAt(offsetInPacketBits,
                                              offsetInPacketBitsLessThan,
                                              offsetInPacketBitsEqual);
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOffsetInPacketBits(const Index offsetInPacketBits) const
{
    return this->_nearestCheckpointBefore(offsetInPacketBits,
                                          offsetInPacketBitsLessThan);
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterOffsetInPacketBits(const Index offsetInPacketBits) const
{
    return this->_nearestCheckpointAfter(offsetInPacketBits,
                                         [](const auto offsetInPacketBits,
                                            const auto& checkpoint) {
        return offsetInPacketBits < checkpoint.first->segment().offsetInPacketBits();
    });
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtTimestamp(const Timestamp& ts) const
{
    return this->nearestCheckpointBeforeOrAtNsFromEpoch(ts.nsFromOrigin());
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeTimestamp(const Timestamp& ts) const
{
    return this->nearestCheckpointBeforeNsFromEpoch(ts.nsFromOrigin());
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterTimestamp(const Timestamp& ts) const
{
    return this->nearestCheckpointAfterNsFromEpoch(ts.nsFromOrigin());
}

static bool nsFromOriginLessThan(const PacketCheckpoints::Checkpoint& checkpoint,
                                 const long long nsFromOrigin)
{
    const auto& eventRecord = *checkpoint.first;

    if (!eventRecord.firstTimestamp()) {
        return false;
    }

    return eventRecord.firstTimestamp()->nsFromOrigin() < nsFromOrigin;
}

static bool nsFromOriginEqual(const EventRecord& eventRecord,
                              const long long nsFromOrigin)
{
    if (!eventRecord.firstTimestamp()) {
        return false;
    }

    return eventRecord.firstTimestamp()->nsFromOrigin() == nsFromOrigin;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtNsFromEpoch(const long long nsFromOrigin) const
{
    const auto checkpoint = this->_nearestCheckpointBeforeOrAt(nsFromOrigin,
                                                               nsFromOriginLessThan,
                                                               nsFromOriginEqual);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->firstTimestamp()) {
        // checkpoint's event record does not even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeNsFromEpoch(const long long nsFromOrigin) const
{
    const auto checkpoint = this->_nearestCheckpointBefore(nsFromOrigin,
                                                           nsFromOriginLessThan);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->firstTimestamp()) {
        // checkpoint's event record does not even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterNsFromEpoch(const long long nsFromOrigin) const
{
    const auto checkpoint = this->_nearestCheckpointAfter(nsFromOrigin,
                                                          [](const auto nsFromOrigin,
                                                             const auto& checkpoint) {
        const auto& eventRecord = *checkpoint.first;

        if (!eventRecord.firstTimestamp()) {
            return false;
        }

        return nsFromOrigin < eventRecord.firstTimestamp()->nsFromOrigin();
    });

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->firstTimestamp()) {
        // checkpoint's event record does not even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

} // namespace jacques
