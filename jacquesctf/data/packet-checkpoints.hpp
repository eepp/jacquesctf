/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PACKET_CHECKPOINTS_HPP
#define _JACQUES_DATA_PACKET_CHECKPOINTS_HPP

#include <cassert>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>
#include <boost/optional.hpp>
#include <yactfr/element-sequence.hpp>
#include <yactfr/element-sequence-iterator.hpp>
#include <yactfr/element-sequence-iterator-position.hpp>
#include <yactfr/decoding-errors.hpp>

#include "aliases.hpp"
#include "data/data-size.hpp"
#include "data/event-record.hpp"
#include "data/timestamp.hpp"
#include "data/packet-checkpoints-build-listener.hpp"
#include "data/metadata.hpp"

namespace jacques {

class PacketDecodingError
{
public:
    explicit PacketDecodingError(const yactfr::DecodingError& decodingError,
                                 const PacketIndexEntry& packetIndexEntry);

    const yactfr::DecodingError& decodingError() const noexcept
    {
        return _decodingError;
    }

    const PacketIndexEntry& packetIndexEntry() const noexcept
    {
        return *_packetIndexEntry;
    }

private:
    yactfr::DecodingError _decodingError;
    const PacketIndexEntry *_packetIndexEntry;
};

class PacketCheckpoints
{
public:
    using Checkpoint = std::pair<EventRecord::SP,
                                 yactfr::ElementSequenceIteratorPosition>;
    using Checkpoints = std::vector<Checkpoint>;

public:
    explicit PacketCheckpoints(yactfr::ElementSequence& seq,
                               const Metadata& metadata,
                               const PacketIndexEntry& packetIndexEntry, Size step,
                               PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    const Checkpoint *nearestCheckpointBeforeOrAtIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointBeforeIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointAfterIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointBeforeOrAtOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointBeforeOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointAfterOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointBeforeOrAtNsFromOrigin(long long nsFromOrigin) const;
    const Checkpoint *nearestCheckpointBeforeNsFromOrigin(long long nsFromOrigin) const;
    const Checkpoint *nearestCheckpointAfterNsFromOrigin(long long nsFromOrigin) const;
    const Checkpoint *nearestCheckpointBeforeOrAtCycles(unsigned long long cycles) const;
    const Checkpoint *nearestCheckpointBeforeCycles(unsigned long long cycles) const;
    const Checkpoint *nearestCheckpointAfterCycles(unsigned long long cycles) const;

    EventRecord::SP nearestEventRecordBeforeOrAtIndex(const Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeIndex(const Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointBeforeIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterIndex(const Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointAfterIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtOffsetInPacketBits(const Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOffsetInPacketBits(const Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterOffsetInPacketBits(const Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointAfterOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtNsFromOrigin(const long long nsFromOrigin) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeNsFromOrigin(const long long nsFromOrigin) const
    {
        auto checkpoint = this->nearestCheckpointBeforeNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterNsFromOrigin(const long long nsFromOrigin) const
    {
        auto checkpoint = this->nearestCheckpointAfterNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtCycles(const unsigned long long cycles) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeCycles(const unsigned long long cycles) const
    {
        auto checkpoint = this->nearestCheckpointBeforeCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterCycles(const unsigned long long cycles) const
    {
        auto checkpoint = this->nearestCheckpointAfterCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP firstEventRecord() const
    {
        assert(!_checkpoints.empty());
        return _checkpoints.front().first;
    }

    EventRecord::SP lastEventRecord() const
    {
        assert(!_checkpoints.empty());
        return _checkpoints.back().first;
    }

    const Checkpoint& firstCheckpoint() const
    {
        assert(!_checkpoints.empty());
        return _checkpoints.front();
    }

    const Checkpoint& lastCheckpoint() const
    {
        assert(!_checkpoints.empty());
        return _checkpoints.back();
    }

    Size eventRecordCount() const noexcept
    {
        if (_checkpoints.empty()) {
            return 0;
        }

        return _checkpoints.back().first->indexInPacket() + 1;
    }

    bool isEmpty() const noexcept
    {
        return _checkpoints.empty();
    }

    const Checkpoints& checkpoints() const noexcept
    {
        return _checkpoints;
    }

    Checkpoints::const_iterator begin() const
    {
        return std::begin(_checkpoints);
    }

    Checkpoints::const_iterator end() const
    {
        return std::end(_checkpoints);
    }

    const boost::optional<PacketDecodingError>& error() const noexcept
    {
        return _error;
    }

private:
    void _createCheckpoint(yactfr::ElementSequenceIterator& it,
                           const Metadata& metadata,
                           const PacketIndexEntry& packetIndexEntry,
                           Index indexInPacket,
                           PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    void _createCheckpoints(yactfr::ElementSequenceIterator& it,
                            const Metadata& metadata,
                            const PacketIndexEntry& packetIndexEntry,
                            Size step,
                            PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    void _tryCreateCheckpoints(yactfr::ElementSequence& seq,
                               const Metadata& metadata,
                               const PacketIndexEntry& packetIndexEntry,
                               Size step,
                               PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    void _lastEventRecordPositions(yactfr::ElementSequenceIteratorPosition& lastPos,
                                   yactfr::ElementSequenceIteratorPosition& penultimatePos,
                                   Index& lastIndexInPacket,
                                   Index& penultimateIndexInPacket,
                                   yactfr::ElementSequenceIterator& it);

    template <typename PropT, typename LessThanFuncT>
    const Checkpoint *_nearestCheckpointAfter(const PropT& prop,
                                              const LessThanFuncT lessThanFunc) const
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        auto it = std::upper_bound(std::begin(_checkpoints),
                                   std::end(_checkpoints), prop, lessThanFunc);

        if (it == std::end(_checkpoints)) {
            // nothing after
            return nullptr;
        }

        return &(*it);
    }

    template <typename PropT, typename LessThanFuncT, typename EqualFuncT>
    const Checkpoint *_nearestCheckpointBeforeOrAt(const PropT& prop,
                                                   const LessThanFuncT lessThanFunc,
                                                   const EqualFuncT equalFunc) const
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        // TODO: probably better to use std::upper_bound() here
        auto it = std::lower_bound(std::begin(_checkpoints),
                                   std::end(_checkpoints), prop, lessThanFunc);

        if (it == std::end(_checkpoints)) {
            // >= not found, but we could still want the last checkpoint
            --it;

            // no need to check for equality because std::lower_bound() did it
            if (lessThanFunc(*it, prop)) {
                return &(*it);
            }

            return nullptr;
        }

        const auto& eventRecord = *it->first;

        if (equalFunc(eventRecord, prop)) {
            // equal
            return &(*it);
        }

        if (it == std::begin(_checkpoints)) {
            // nothing before
            return nullptr;
        }

        --it;
        return &(*it);
    }

    template <typename PropT, typename LessThanFuncT>
    const Checkpoint *_nearestCheckpointBefore(const PropT& prop,
                                               const LessThanFuncT lessThanFunc) const
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        auto it = std::lower_bound(std::begin(_checkpoints),
                                   std::end(_checkpoints), prop, lessThanFunc);

        if (it == std::end(_checkpoints)) {
            // >= not found, but we could still want the last checkpoint
            --it;

            if (lessThanFunc(*it, prop)) {
                return &(*it);
            }

            return nullptr;
        }

        if (it == std::begin(_checkpoints)) {
            // nothing strictly before
            return nullptr;
        }

        --it;
        return &(*it);
    }

private:
    Checkpoints _checkpoints;
    boost::optional<PacketDecodingError> _error;
    boost::optional<Index> _packetContextOffsetInPacketBits;
};

} // namespace jacques

#endif // _JACQUES_DATA_PACKET_CHECKPOINTS_HPP
