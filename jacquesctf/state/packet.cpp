/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>

#include "packet.hpp"

namespace jacques {

Packet::Packet(const PacketIndexEntry& indexEntry,
               yactfr::PacketSequence& seq,
               const Metadata& metadata,
               yactfr::DataSource::UP dataSrc,
               PacketCheckpointsBuildListener& packetCheckpointsBuildListener) :
    _indexEntry {&indexEntry},
    _seq {&seq},
    _metadata {&metadata},
    _dataSrc {std::move(dataSrc)},
    _it {std::begin(seq)},
    _endIt {std::end(seq)},
    _checkpoints {
        seq, metadata, *_indexEntry, 49999, packetCheckpointsBuildListener,
    },
    _eventRecordCacheMaxSize {500}
{
}

boost::optional<Index> Packet::_dataRegionIndexInCache(const Index offsetInPacketBits)
{
    auto it = std::lower_bound(std::begin(_dataRegionCache),
                               std::end(_dataRegionCache),
                               offsetInPacketBits,
                               [](const auto& dataRegionSp,
                                  const auto& offsetInPacketBits) {
        return dataRegionSp->segment().offsetInPacketBits() < offsetInPacketBits;
    });

    if (it == std::end(_dataRegionCache)) {
        return boost::none;
    }

    const auto& dataRegionSp = *it;

    if (it == std::begin(_dataRegionCache)) {
        return boost::none;
    }

    if (dataRegionSp->segment().offsetInPacketBits() > offsetInPacketBits) {
        --it;
    }

    return std::distance(std::begin(_dataRegionCache), it);
}

boost::optional<Index> Packet::_eventRecordIndexInCacheByOffsetInPacketBits(const Index offsetInPacketBits)
{
    auto it = std::lower_bound(std::begin(_eventRecordCache),
                               std::end(_eventRecordCache),
                               offsetInPacketBits,
                               [](const auto& eventRecordSp,
                                  const auto& offsetInPacketBits) {
        return eventRecordSp->segment().offsetInPacketBits() < offsetInPacketBits;
    });

    if (it == std::end(_eventRecordCache)) {
        return boost::none;
    }

    const auto& eventRecordSp = *it;

    if (it == std::begin(_eventRecordCache)) {
        return boost::none;
    }

    if (eventRecordSp->segment().offsetInPacketBits() > offsetInPacketBits) {
        --it;
    }

    return std::distance(std::begin(_eventRecordCache), it);
}

void Packet::eventRecordsAtIndexInPacket(Index reqIndexInPacket,
                                         Size reqCount,
                                         std::vector<EventRecord::SP>& eventRecords)
{
    assert(reqIndexInPacket + reqCount <= _checkpoints.eventRecordCount());

    while (reqCount > 0) {
        this->_cacheEventRecordsAtIndexInPacket(reqIndexInPacket);

        const auto cacheBeginIndex = this->_eventRecordIndexInCacheFromIndexInPacket(reqIndexInPacket);
        const auto copyCount = std::min(reqCount,
                                        _eventRecordCache.size() - cacheBeginIndex);

        assert(cacheBeginIndex + copyCount <= _eventRecordCache.size());

        const auto cacheBeginIt = std::begin(_eventRecordCache) +
                                  cacheBeginIndex;

        std::copy(cacheBeginIt, cacheBeginIt + copyCount,
                  std::back_inserter(eventRecords));
        reqCount -= copyCount;
        reqIndexInPacket += copyCount;
    }
}

const EventRecord& Packet::eventRecordAtIndexInPacket(const Index reqIndexInPacket)
{
    assert(reqIndexInPacket < _checkpoints.eventRecordCount());
    this->_cacheEventRecordsAtIndexInPacket(reqIndexInPacket);
    return *_eventRecordCache[this->_eventRecordIndexInCacheFromIndexInPacket(reqIndexInPacket)];
}

void Packet::_cacheEventRecordsAtIndexInPacket(const Index reqIndexInPacket)
{
    const auto baseIndex = this->_eventRecordBaseIndexInCacheFromIndexInPacket(reqIndexInPacket);

    if (!_eventRecordCache.empty() &&
            _eventRecordCache.front()->indexInPacket() == baseIndex) {
        // already in cache
        return;
    }

    // are we asking to cache what immediately follows the current cache?
    if (!_eventRecordCache.empty()) {
        if (baseIndex == _eventRecordCache.front()->indexInPacket() + _eventRecordCacheMaxSize) {
            _it.restorePosition(_posAfterEventRecordCache);
            this->_cacheEventRecordsFromCurIt(baseIndex);
            return;
        }
    }

    // find nearest event record checkpoint
    auto cp = _checkpoints.nearestCheckpointBeforeOrAtIndex(baseIndex);

    assert(cp);

    auto curIndex = cp->first->indexInPacket();

    _it.restorePosition(cp->second);

    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (curIndex == baseIndex) {
                this->_cacheEventRecordsFromCurIt(baseIndex);
                return;
            }

            ++curIndex;
        }

        ++_it;
    }
}

void Packet::_cacheEventRecordsFromCurIt(Index indexInPacket)
{
    assert(this->_eventRecordBaseIndexInCacheFromIndexInPacket(indexInPacket) == indexInPacket);
    _eventRecordCache.clear();

    const auto count = std::min(_eventRecordCacheMaxSize,
                                _checkpoints.eventRecordCount() - indexInPacket);
    const auto offsetInDataStreamBytes = _indexEntry->offsetInDataStreamBytes();
    Index i = 0;

    while (i < count) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            auto eventRecord = EventRecord::createFromPacketSequenceIterator(_it,
                                                                             *_metadata,
                                                                             offsetInDataStreamBytes,
                                                                             indexInPacket);
            _eventRecordCache.push_back(std::move(eventRecord));
            ++indexInPacket;
            ++i;
            assert(_it->kind() == yactfr::Element::Kind::EVENT_RECORD_END);
            continue;
        }

        ++_it;
    }

    assert(_it->kind() == yactfr::Element::Kind::EVENT_RECORD_END);
    _it.savePosition(_posAfterEventRecordCache);
}

} // namespace jacques

