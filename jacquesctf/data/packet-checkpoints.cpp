/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "packet-checkpoints.hpp"
#include "logging.hpp"

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
    theLogger->debug("Packet checkpoints: {}.", _checkpoints.size());
}

void PacketCheckpoints::_tryCreateCheckpoints(yactfr::PacketSequence& seq,
                                              const Metadata& metadata,
                                              const PacketIndexEntry& packetIndexEntry,
                                              const Size step,
                                              PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
    auto it = seq.at(packetIndexEntry.offsetInDataStreamBytes());

    // we consider other errors (e.g., I/O) unrecoverable: do not catch them
    try {
        this->_createCheckpoints(it, metadata, packetIndexEntry,
                                 step, packetCheckpointsBuildListener);
    } catch (const yactfr::DecodingError& ex) {
        _error = PacketDecodingError {ex, packetIndexEntry};
    }

    /*
     * Try to set a checkpoint for the last event record, even if we got
     * an error (or get one trying to do so), so as to guarantee as many
     * event records as possible between the first and last checkpoints.
     *
     * The reason we request both the last position and the penultimate
     * one is because, while trying to create a checkpoint with
     * _createCheckpoint() using the last position, the iterator could
     * throw before we have the required information to create a
     * complete event record object. In that case, we know that at least
     * the penultimate one is complete, so fall back to it.
     *
     * Also _lastEventRecordPositions() does not catch any exception
     * because we want to do it here.
     */
    yactfr::PacketSequenceIteratorPosition lastPos;
    yactfr::PacketSequenceIteratorPosition penultimatePos;
    Index lastIndex = 0;
    Index penultimateIndex = 0;

    try {
        this->_lastEventRecordPositions(lastPos, penultimatePos, lastIndex,
                                        penultimateIndex, it);
    } catch (const yactfr::DecodingError& ex) {
        _error = PacketDecodingError {ex, packetIndexEntry};
    }

    if (!lastPos || lastPos == _checkpoints.back().second) {
        // we got everything possible: do not duplicate
        return;
    }

    it.restorePosition(lastPos);

    try {
        this->_createCheckpoint(it, metadata, packetIndexEntry,
                                lastIndex, packetCheckpointsBuildListener);
    } catch (const yactfr::DecodingError& ex) {
        assert(_error);

        if (!penultimatePos || penultimatePos == _checkpoints.back().second) {
            // we got everything possible: do not duplicate
            return;
        }

        // this won't fail
        it.restorePosition(penultimatePos);
        this->_createCheckpoint(it, metadata, packetIndexEntry,
                                penultimateIndex,
                                packetCheckpointsBuildListener);
    }
}

void PacketCheckpoints::_createCheckpoints(yactfr::PacketSequenceIterator& it,
                                           const Metadata& metadata,
                                           const PacketIndexEntry& packetIndexEntry,
                                           const Size step,
                                           PacketCheckpointsBuildListener& packetCheckpointsBuildListener)
{
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
}

void PacketCheckpoints::_lastEventRecordPositions(yactfr::PacketSequenceIteratorPosition& lastPos,
                                                  yactfr::PacketSequenceIteratorPosition& penultimatePos,
                                                  Index& lastIndexInPacket,
                                                  Index& penultimateIndexInPacket,
                                                  yactfr::PacketSequenceIterator& it)
{
    // find last event record and create a checkpoint if not already done
    if (_checkpoints.empty()) {
        // no event records in this packet!
        return;
    }

    it.restorePosition(_checkpoints.back().second);

    auto nextIndexInPacket = _checkpoints.back().first->indexInPacket();

    while (it->kind() != yactfr::Element::Kind::PACKET_END) {
        if (it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            penultimatePos = std::move(lastPos);
            it.savePosition(lastPos);
            lastIndexInPacket = nextIndexInPacket;
            penultimateIndexInPacket = nextIndexInPacket - 1;
            ++nextIndexInPacket;
        }

        ++it;
    }
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

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtNsFromOrigin(const long long nsFromOrigin) const
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

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeNsFromOrigin(const long long nsFromOrigin) const
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

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterNsFromOrigin(const long long nsFromOrigin) const
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

static bool cyclesLessThan(const PacketCheckpoints::Checkpoint& checkpoint,
                           const unsigned long long cycles)
{
    const auto& eventRecord = *checkpoint.first;

    if (!eventRecord.firstTimestamp()) {
        return false;
    }

    return eventRecord.firstTimestamp()->cycles() < cycles;
}

static bool cyclesEqual(const EventRecord& eventRecord,
                        const unsigned long long cycles)
{
    if (!eventRecord.firstTimestamp()) {
        return false;
    }

    return eventRecord.firstTimestamp()->cycles() == cycles;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeOrAtCycles(const unsigned long long cycles) const
{
    const auto checkpoint = this->_nearestCheckpointBeforeOrAt(cycles,
                                                               cyclesLessThan,
                                                               cyclesEqual);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->firstTimestamp()) {
        // checkpoint's event record does not even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointBeforeCycles(const unsigned long long cycles) const
{
    const auto checkpoint = this->_nearestCheckpointBefore(cycles,
                                                           cyclesLessThan);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->firstTimestamp()) {
        // checkpoint's event record does not even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PacketCheckpoints::Checkpoint *PacketCheckpoints::nearestCheckpointAfterCycles(const unsigned long long cycles) const
{
    const auto checkpoint = this->_nearestCheckpointAfter(cycles,
                                                          [](const auto cycles,
                                                             const auto& checkpoint) {
        const auto& eventRecord = *checkpoint.first;

        if (!eventRecord.firstTimestamp()) {
            return false;
        }

        return cycles < eventRecord.firstTimestamp()->cycles();
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
