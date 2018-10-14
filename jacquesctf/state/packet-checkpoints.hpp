/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_CHECKPOINTS_HPP
#define _JACQUES_PACKET_CHECKPOINTS_HPP

#include <cassert>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>
#include <boost/optional.hpp>
#include <yactfr/packet-sequence.hpp>
#include <yactfr/packet-sequence-iterator.hpp>
#include <yactfr/packet-sequence-iterator-position.hpp>
#include <yactfr/decoding-errors.hpp>

#include "aliases.hpp"
#include "event-record.hpp"
#include "timestamp.hpp"
#include "packet-checkpoints-build-listener.hpp"
#include "metadata.hpp"

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
                                 yactfr::PacketSequenceIteratorPosition>;
    using Checkpoints = std::vector<Checkpoint>;

public:
    explicit PacketCheckpoints(yactfr::PacketSequence& seq,
                               const Metadata& metadata,
                               const PacketIndexEntry& packetIndexEntry, Size step,
                               PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    const Checkpoint *nearestCheckpointBeforeOrAtIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointBeforeIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointAfterIndex(Index indexInPacket) const;
    const Checkpoint *nearestCheckpointBeforeOrAtOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointBeforeOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointAfterOffsetInPacketBits(Index offsetInPacketBits) const;
    const Checkpoint *nearestCheckpointBeforeOrAtTimestamp(const Timestamp& ts) const;
    const Checkpoint *nearestCheckpointBeforeTimestamp(const Timestamp& ts) const;
    const Checkpoint *nearestCheckpointAfterTimestamp(const Timestamp& ts) const;
    const Checkpoint *nearestCheckpointBeforeOrAtNsFromEpoch(long long nsFromEpoch) const;
    const Checkpoint *nearestCheckpointBeforeNsFromEpoch(long long nsFromEpoch) const;
    const Checkpoint *nearestCheckpointAfterNsFromEpoch(long long nsFromEpoch) const;

    EventRecord::SP nearestEventRecordBeforeOrAtIndex(Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeIndex(Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointBeforeIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterIndex(Index indexInPacket) const
    {
        auto checkpoint = this->nearestCheckpointAfterIndex(indexInPacket);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtOffsetInPacketBits(Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOffsetInPacketBits(Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterOffsetInPacketBits(Index offsetInPacketBits) const
    {
        auto checkpoint = this->nearestCheckpointAfterOffsetInPacketBits(offsetInPacketBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtTimestamp(const Timestamp& ts) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtTimestamp(ts);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeTimestamp(const Timestamp& ts) const
    {
        auto checkpoint = this->nearestCheckpointBeforeTimestamp(ts);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterTimestamp(const Timestamp& ts) const
    {
        auto checkpoint = this->nearestCheckpointAfterTimestamp(ts);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeOrAtNsFromEpoch(long long nsFromEpoch) const
    {
        auto checkpoint = this->nearestCheckpointBeforeOrAtNsFromEpoch(nsFromEpoch);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordBeforeNsFromEpoch(long long nsFromEpoch) const
    {
        auto checkpoint = this->nearestCheckpointBeforeNsFromEpoch(nsFromEpoch);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    EventRecord::SP nearestEventRecordAfterNsFromEpoch(long long nsFromEpoch) const
    {
        auto checkpoint = this->nearestCheckpointAfterNsFromEpoch(nsFromEpoch);

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
    void _createCheckpoint(yactfr::PacketSequenceIterator& it,
                           const Metadata& metadata,
                           const PacketIndexEntry& packetIndexEntry,
                           Index indexInPacket,
                           PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    void _createCheckpoints(yactfr::PacketSequence& seq,
                            const Metadata& metadata,
                            const PacketIndexEntry& packetIndexEntry,
                            Size step,
                            PacketCheckpointsBuildListener& packetCheckpointsBuildListener);
    void _tryCreateCheckpoints(yactfr::PacketSequence& seq,
                               const Metadata& metadata,
                               const PacketIndexEntry& packetIndexEntry,
                               Size step,
                               PacketCheckpointsBuildListener& packetCheckpointsBuildListener);

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
};

} // namespace jacques

#endif // _JACQUES_PACKET_CHECKPOINTS_HPP
