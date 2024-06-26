/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "pkt-checkpoints.hpp"

namespace jacques {

PktDecodingError::PktDecodingError(const yactfr::DecodingError& decodingError,
                                   const PktIndexEntry& pktIndexEntry) :
    _decodingError {decodingError},
    _pktIndexEntry {&pktIndexEntry}
{
}

PktCheckpoints::PktCheckpoints(yactfr::ElementSequence& seq, const Metadata& metadata,
                               const PktIndexEntry& pktIndexEntry, const Size step,
                               PktCheckpointsBuildListener& pktCheckpointsBuildListener)
{
    this->_tryCreateCheckpoints(seq, metadata, pktIndexEntry, step, pktCheckpointsBuildListener);
}

void PktCheckpoints::_tryCreateCheckpoints(yactfr::ElementSequence& seq, const Metadata& metadata,
                                           const PktIndexEntry& pktIndexEntry, const Size step,
                                           PktCheckpointsBuildListener& pktCheckpointsBuildListener)
{
    auto it = seq.at(pktIndexEntry.offsetInDsFileBytes());

    // we consider other errors (e.g., I/O) unrecoverable: do not catch them
    try {
        this->_createCheckpoints(it, metadata, pktIndexEntry, step, pktCheckpointsBuildListener);
    } catch (const yactfr::DecodingError& exc) {
        _error = PktDecodingError {exc, pktIndexEntry};
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
     * Also _lastErPositions() doesn't catch any exception because we
     * want to do it here.
     */
    yactfr::ElementSequenceIteratorPosition lastPos;
    yactfr::ElementSequenceIteratorPosition penultimatePos;
    Index lastIndex = 0;
    Index penultimateIndex = 0;

    try {
        this->_lastErPositions(lastPos, penultimatePos, lastIndex, penultimateIndex, it);
    } catch (const yactfr::DecodingError& exc) {
        _error = PktDecodingError {exc, pktIndexEntry};
    }

    if (!lastPos || lastPos == _checkpoints.back().second) {
        // we got everything possible: do not duplicate
        return;
    }

    it.restorePosition(lastPos);

    try {
        this->_createCheckpoint(it, metadata, pktIndexEntry, lastIndex,
                                pktCheckpointsBuildListener);
    } catch (const yactfr::DecodingError& exc) {
        assert(_error);

        if (!penultimatePos || penultimatePos == _checkpoints.back().second) {
            // we got everything possible: do not duplicate
            return;
        }

        // this won't fail
        it.restorePosition(penultimatePos);
        this->_createCheckpoint(it, metadata, pktIndexEntry, penultimateIndex,
                                pktCheckpointsBuildListener);
    }
}

void PktCheckpoints::_createCheckpoints(yactfr::ElementSequenceIterator& it,
                                        const Metadata& metadata,
                                        const PktIndexEntry& pktIndexEntry, const Size step,
                                        PktCheckpointsBuildListener& pktCheckpointsBuildListener)
{
    Index indexInPkt = 0;

    // create all checkpoints except (possibly) the last one
    while (it->kind() != yactfr::Element::Kind::PacketEnd) {
        if (it->kind() == yactfr::Element::Kind::EventRecordBeginning) {
            const auto curIndexInPkt = indexInPkt;

            ++indexInPkt;

            if (curIndexInPkt % step == 0) {
                this->_createCheckpoint(it, metadata, pktIndexEntry, curIndexInPkt,
                                        pktCheckpointsBuildListener);
                continue;
            }
        }

        ++it;
    }
}

void PktCheckpoints::_lastErPositions(yactfr::ElementSequenceIteratorPosition& lastPos,
                                      yactfr::ElementSequenceIteratorPosition& penultimatePos,
                                      Index& lastIndexInPkt, Index& penultimateIndexInPkt,
                                      yactfr::ElementSequenceIterator& it)
{
    // find last event record and create a checkpoint if not already done
    if (_checkpoints.empty()) {
        // no event records in this packet!
        return;
    }

    it.restorePosition(_checkpoints.back().second);

    auto nextIndexInPkt = _checkpoints.back().first->indexInPkt();

    while (it->kind() != yactfr::Element::Kind::PacketEnd) {
        if (it->kind() == yactfr::Element::Kind::EventRecordBeginning) {
            penultimatePos = std::move(lastPos);
            it.savePosition(lastPos);
            lastIndexInPkt = nextIndexInPkt;
            penultimateIndexInPkt = nextIndexInPkt - 1;
            ++nextIndexInPkt;
        }

        ++it;
    }
}

void PktCheckpoints::_createCheckpoint(yactfr::ElementSequenceIterator& it,
                                       const Metadata& metadata,
                                       const PktIndexEntry& pktIndexEntry, const Index indexInPkt,
                                       PktCheckpointsBuildListener& pktCheckpointsBuildListener)
{
    yactfr::ElementSequenceIteratorPosition pos;

    assert(it->kind() == yactfr::Element::Kind::EventRecordBeginning);
    it.savePosition(pos);

    const auto er = Er::createFromElemSeqIt(it, metadata, pktIndexEntry, indexInPkt);
    _checkpoints.push_back({er, std::move(pos)});
    pktCheckpointsBuildListener.update(*er);
}

namespace {

bool indexInPktLessThan(const PktCheckpoints::Checkpoint& checkpoint,
                               const Index indexInPkt)
{
    return checkpoint.first->indexInPkt() < indexInPkt;
}

bool indexInPktEqual(const Er& er, const Index indexInPkt)
{
    return er.indexInPkt() == indexInPkt;
}

} // namespace

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeOrAtIndex(const Index indexInPkt) const noexcept
{
    return this->_nearestCheckpointBeforeOrAt(indexInPkt, indexInPktLessThan, indexInPktEqual);
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeIndex(const Index indexInPkt) const noexcept
{
    return this->_nearestCheckpointBefore(indexInPkt, indexInPktLessThan);
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointAfterIndex(const Index indexInPkt) const noexcept
{
    return this->_nearestCheckpointAfter(indexInPkt,
                                         [](const auto indexInPkt, const auto& checkpoint) {
        return indexInPkt < checkpoint.first->indexInPkt();
    });
}

namespace {

bool offsetInPktBitsLessThan(const PktCheckpoints::Checkpoint& checkpoint,
                                    const Index offsetInPktBits)
{
    return checkpoint.first->segment().offsetInPktBits() < offsetInPktBits;
}

bool offsetInPktBitsEqual(const Er& er, const Index offsetInPktBits)
{
    return er.segment().offsetInPktBits() == offsetInPktBits;
}

} // namespace

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeOrAtOffsetInPktBits(const Index offsetInPktBits) const noexcept
{
    return this->_nearestCheckpointBeforeOrAt(offsetInPktBits, offsetInPktBitsLessThan,
                                              offsetInPktBitsEqual);
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeOffsetInPktBits(const Index offsetInPktBits) const noexcept
{
    return this->_nearestCheckpointBefore(offsetInPktBits, offsetInPktBitsLessThan);
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointAfterOffsetInPktBits(const Index offsetInPktBits) const noexcept
{
    return this->_nearestCheckpointAfter(offsetInPktBits,
                                         [](const auto offsetInPktBits, const auto& checkpoint) {
        return offsetInPktBits < checkpoint.first->segment().offsetInPktBits();
    });
}

namespace {

bool nsFromOriginLessThan(const PktCheckpoints::Checkpoint& checkpoint,
                          const long long nsFromOrigin)
{
    const auto& er = *checkpoint.first;

    if (!er.ts()) {
        return false;
    }

    return er.ts()->nsFromOrigin() < nsFromOrigin;
}

bool nsFromOriginEqual(const Er& er, const long long nsFromOrigin)
{
    if (!er.ts()) {
        return false;
    }

    return er.ts()->nsFromOrigin() == nsFromOrigin;
}

} // namespace

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeOrAtNsFromOrigin(const long long nsFromOrigin) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointBeforeOrAt(nsFromOrigin, nsFromOriginLessThan,
                                                               nsFromOriginEqual);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeNsFromOrigin(const long long nsFromOrigin) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointBefore(nsFromOrigin, nsFromOriginLessThan);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointAfterNsFromOrigin(const long long nsFromOrigin) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointAfter(nsFromOrigin,
                                                          [](const auto nsFromOrigin,
                                                             const auto& checkpoint) {
        const auto& er = *checkpoint.first;

        if (!er.ts()) {
            return false;
        }

        return nsFromOrigin < er.ts()->nsFromOrigin();
    });

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

namespace {

bool cyclesLessThan(const PktCheckpoints::Checkpoint& checkpoint, const unsigned long long cycles)
{
    const auto& er = *checkpoint.first;

    if (!er.ts()) {
        return false;
    }

    return er.ts()->cycles() < cycles;
}

bool cyclesEqual(const Er& er, const unsigned long long cycles)
{
    if (!er.ts()) {
        return false;
    }

    return er.ts()->cycles() == cycles;
}

} // namespace

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeOrAtCycles(const unsigned long long cycles) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointBeforeOrAt(cycles, cyclesLessThan, cyclesEqual);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointBeforeCycles(const unsigned long long cycles) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointBefore(cycles, cyclesLessThan);

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

const PktCheckpoints::Checkpoint *PktCheckpoints::nearestCheckpointAfterCycles(const unsigned long long cycles) const noexcept
{
    const auto checkpoint = this->_nearestCheckpointAfter(cycles,
                                                          [](const auto cycles,
                                                             const auto& checkpoint) {
        const auto& er = *checkpoint.first;

        if (!er.ts()) {
            return false;
        }

        return cycles < er.ts()->cycles();
    });

    if (!checkpoint) {
        return nullptr;
    }

    if (!checkpoint->first->ts()) {
        // event record of checkpoint doesn't even have a timestamp
        return nullptr;
    }

    return checkpoint;
}

} // namespace jacques
