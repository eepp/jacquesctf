/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_CHECKPOINTS_HPP
#define _JACQUES_DATA_PKT_CHECKPOINTS_HPP

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
#include "data-len.hpp"
#include "er.hpp"
#include "ts.hpp"
#include "pkt-checkpoints-build-listener.hpp"
#include "metadata.hpp"

namespace jacques {

class PktDecodingError final
{
public:
    explicit PktDecodingError(const yactfr::DecodingError& decodingError,
                              const PktIndexEntry& pktIndexEntry);

    const yactfr::DecodingError& decodingError() const noexcept
    {
        return _decodingError;
    }

    const PktIndexEntry& pktIndexEntry() const noexcept
    {
        return *_pktIndexEntry;
    }

private:
    yactfr::DecodingError _decodingError;
    const PktIndexEntry *_pktIndexEntry;
};

class PktCheckpoints final
{
public:
    using Checkpoint = std::pair<Er::SP, yactfr::ElementSequenceIteratorPosition>;
    using Checkpoints = std::vector<Checkpoint>;

public:
    explicit PktCheckpoints(yactfr::ElementSequence& seq, const Metadata& metadata,
                            const PktIndexEntry& pktIndexEntry, Size step,
                            PktCheckpointsBuildListener& pktCheckpointsBuildListener);

    const Checkpoint *nearestCheckpointBeforeOrAtIndex(Index indexInPkt) const noexcept;
    const Checkpoint *nearestCheckpointBeforeIndex(Index indexInPkt) const noexcept;
    const Checkpoint *nearestCheckpointAfterIndex(Index indexInPkt) const noexcept;
    const Checkpoint *nearestCheckpointBeforeOrAtOffsetInPktBits(Index offsetInPktBits) const noexcept;
    const Checkpoint *nearestCheckpointBeforeOffsetInPktBits(Index offsetInPktBits) const noexcept;
    const Checkpoint *nearestCheckpointAfterOffsetInPktBits(Index offsetInPktBits) const noexcept;
    const Checkpoint *nearestCheckpointBeforeOrAtNsFromOrigin(long long nsFromOrigin) const noexcept;
    const Checkpoint *nearestCheckpointBeforeNsFromOrigin(long long nsFromOrigin) const noexcept;
    const Checkpoint *nearestCheckpointAfterNsFromOrigin(long long nsFromOrigin) const noexcept;
    const Checkpoint *nearestCheckpointBeforeOrAtCycles(unsigned long long cycles) const noexcept;
    const Checkpoint *nearestCheckpointBeforeCycles(unsigned long long cycles) const noexcept;
    const Checkpoint *nearestCheckpointAfterCycles(unsigned long long cycles) const noexcept;

    Er::SP nearestErBeforeOrAtIndex(const Index indexInPkt) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeOrAtIndex(indexInPkt);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeIndex(const Index indexInPkt) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeIndex(indexInPkt);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErAfterIndex(const Index indexInPkt) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointAfterIndex(indexInPkt);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeOrAtOffsetInPktBits(const Index offsetInPktBits) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeOrAtOffsetInPktBits(offsetInPktBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeOffsetInPktBits(const Index offsetInPktBits) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeOffsetInPktBits(offsetInPktBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErAfterOffsetInPktBits(const Index offsetInPktBits) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointAfterOffsetInPktBits(offsetInPktBits);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeOrAtNsFromOrigin(const long long nsFromOrigin) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeOrAtNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeNsFromOrigin(const long long nsFromOrigin) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErAfterNsFromOrigin(const long long nsFromOrigin) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointAfterNsFromOrigin(nsFromOrigin);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeOrAtCycles(const unsigned long long cycles) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeOrAtCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErBeforeCycles(const unsigned long long cycles) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointBeforeCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP nearestErAfterCycles(const unsigned long long cycles) const noexcept
    {
        const auto checkpoint = this->nearestCheckpointAfterCycles(cycles);

        if (!checkpoint) {
            return nullptr;
        }

        return checkpoint->first;
    }

    Er::SP firstEr() const noexcept
    {
        assert(!_checkpoints.empty());
        return _checkpoints.front().first;
    }

    Er::SP lastEr() const noexcept
    {
        assert(!_checkpoints.empty());
        return _checkpoints.back().first;
    }

    const Checkpoint& firstCheckpoint() const noexcept
    {
        assert(!_checkpoints.empty());
        return _checkpoints.front();
    }

    const Checkpoint& lastCheckpoint() const noexcept
    {
        assert(!_checkpoints.empty());
        return _checkpoints.back();
    }

    Size erCount() const noexcept
    {
        if (_checkpoints.empty()) {
            return 0;
        }

        return _checkpoints.back().first->indexInPkt() + 1;
    }

    bool isEmpty() const noexcept
    {
        return _checkpoints.empty();
    }

    const Checkpoints& checkpoints() const noexcept
    {
        return _checkpoints;
    }

    Checkpoints::const_iterator begin() const noexcept
    {
        return _checkpoints.begin();
    }

    Checkpoints::const_iterator end() const noexcept
    {
        return _checkpoints.end();
    }

    const boost::optional<PktDecodingError>& error() const noexcept
    {
        return _error;
    }

private:
    void _createCheckpoint(yactfr::ElementSequenceIterator& it, const Metadata& metadata,
                           const PktIndexEntry& pktIndexEntry, Index indexInPkt,
                           PktCheckpointsBuildListener& pktCheckpointsBuildListener);

    void _createCheckpoints(yactfr::ElementSequenceIterator& it, const Metadata& metadata,
                            const PktIndexEntry& pktIndexEntry, Size step,
                            PktCheckpointsBuildListener& pktCheckpointsBuildListener);

    void _tryCreateCheckpoints(yactfr::ElementSequence& seq, const Metadata& metadata,
                               const PktIndexEntry& pktIndexEntry, Size step,
                               PktCheckpointsBuildListener& pktCheckpointsBuildListener);

    void _lastErPositions(yactfr::ElementSequenceIteratorPosition& lastPos,
                          yactfr::ElementSequenceIteratorPosition& penultimatePos,
                          Index& lastIndexInPkt, Index& penultimateIndexInPkt,
                          yactfr::ElementSequenceIterator& it);

    template <typename PropT, typename LtFuncT>
    const Checkpoint *_nearestCheckpointAfter(const PropT& prop, LtFuncT&& ltFunc) const noexcept
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        const auto it = std::upper_bound(_checkpoints.begin(), _checkpoints.end(), prop,
                                         std::forward<LtFuncT>(ltFunc));

        if (it == _checkpoints.end()) {
            // nothing after
            return nullptr;
        }

        return &(*it);
    }

    template <typename PropT, typename LtFuncT, typename EqFuncT>
    const Checkpoint *_nearestCheckpointBeforeOrAt(const PropT& prop, LtFuncT&& ltFunc,
                                                   EqFuncT&& eqFunc) const noexcept
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        // TODO: probably better to use std::upper_bound() here
        auto it = std::lower_bound(_checkpoints.begin(), _checkpoints.end(), prop,
                                   std::forward<LtFuncT>(ltFunc));

        if (it == _checkpoints.end()) {
            // >= not found, but we could still want the last checkpoint
            --it;

            // no need to check for equality because std::lower_bound() did it
            if (std::forward<LtFuncT>(ltFunc)(*it, prop)) {
                return &(*it);
            }

            return nullptr;
        }

        const auto& er = *it->first;

        if (eqFunc(er, prop)) {
            // equal
            return &(*it);
        }

        if (it == _checkpoints.begin()) {
            // nothing before
            return nullptr;
        }

        --it;
        return &(*it);
    }

    template <typename PropT, typename LtFuncT>
    const Checkpoint *_nearestCheckpointBefore(const PropT& prop, LtFuncT&& ltFunc) const noexcept
    {
        if (_checkpoints.empty()) {
            return nullptr;
        }

        auto it = std::lower_bound(_checkpoints.begin(), _checkpoints.end(), prop,
                                   std::forward<LtFuncT>(ltFunc));

        if (it == _checkpoints.end()) {
            // >= not found, but we could still want the last checkpoint
            --it;

            if (ltFunc(*it, prop)) {
                return &(*it);
            }

            return nullptr;
        }

        if (it == _checkpoints.begin()) {
            // nothing strictly before
            return nullptr;
        }

        --it;
        return &(*it);
    }

private:
    Checkpoints _checkpoints;
    boost::optional<PktDecodingError> _error;
    boost::optional<Index> _pktCtxOffsetInPktBits;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_CHECKPOINTS_HPP
