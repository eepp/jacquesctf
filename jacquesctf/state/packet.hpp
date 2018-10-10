/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_HPP
#define _JACQUES_PACKET_HPP

#include <memory>
#include <vector>
#include <yactfr/packet-sequence.hpp>
#include <yactfr/packet-sequence-iterator.hpp>
#include <yactfr/data-source.hpp>

#include "packet-index-entry.hpp"
#include "packet-checkpoints.hpp"
#include "event-record.hpp"
#include "data-region.hpp"
#include "packet-checkpoints-build-listener.hpp"
#include "metadata.hpp"

namespace jacques {

/*
 * This object's purpose is to provide data regions and event records to
 * views. This is the core data required by the packet inspection
 * activity.
 *
 * We keep two separate caches:
 *
 * * Data regions
 * * Event records
 *
 * Each cache is a sorted vector of shared objects. Each cache maps to
 * its original data in the same way: it's a section of the packet
 * starting at a fixed offset (for data regions) or index (for event
 * records), and having a fixed size in bits (for data regions) or
 * number of event records.
 *
 * Suppose that those are the potential sections of event records (cache
 * is empty at this point, and assume 8 event records per section; each
 * `=` is a non-cached event record):
 *
 *     ======== ======== ======== ======== ======== ======== =====
 *
 * This packet contains 53 event records.
 *
 * When the user makes a request with
 * Packet::eventRecordsAtIndexInPacket(), we find to which section this
 * event record belongs. This is a constant time operation: for example,
 * for event record #20, it's the section from event record #8 to event
 * record #15. When this happens, we create event record objects for
 * each event record in the section and this becomes the current cache.
 * For example, after a request of event records from #20 to #22 (`^`
 * means a requested event record, `#` means in cache):
 *
 *     ======== ======== ######## ======== ======== ======== =====
 *                           ^^^
 *
 * Once the event records are cached,
 * Packet::eventRecordsAtIndexInPacket() can find them easily and fill
 * the output vector with the existing shared objects. The next requests
 * for event records within the cached section will be much faster to
 * fulfill as it won't need to touch the packet sequence iterator or
 * allocate any object.
 *
 * If the request spans more than a single section, we only keep the
 * section containing the last requested event records in cache. This is
 * not ideal but it's how it works now. For example, after a request of
 * event records from #6 to #10 (`*` means created, but not currently
 * cached):
 *
 *     ======** ######## ======== ======== ======== ======== =====
 *           ^^ ^^^
 *
 * With an appropriate configuration of section sizes, this will only
 * cause trouble when viewing the border between two sections at the
 * same time, in which case this specific request will be slower than
 * usual. The larger the section size, the less this happens when
 * inspecting a given packet. Note that users (views) can have their own
 * cache too to mitigate this delay.
 *
 * Because elements are sorted in the caches, we can perform a binary
 * search to find a specific element using one of its intrinsically
 * ordered properties (index, offset in packet, timestamp).
 *
 * Caching is valuable here because of how a typical packet inspection
 * session induces a locality of reference: you're either going backward
 * or forward from the offset you're inspecting once you find a location
 * of interest, so there will typically be a lot of cache hits.
 *
 * Next step: have an LRU cache of section caches, so as to respond to
 * the use case of jumping back and forth to a few different offsets
 * located in different sections.
 */
class Packet
{
public:
    using SP = std::shared_ptr<Packet>;

public:
    explicit Packet(const PacketIndexEntry& indexEntry,
                    yactfr::PacketSequence& seq,
                    const Metadata& metadata,
                    yactfr::DataSource::UP dataSrc,
                    PacketCheckpointsBuildListener& packetCheckpointsBuildListener);

    const PacketIndexEntry& indexEntry() const noexcept
    {
        return *_indexEntry;
    }

    Size eventRecordCount() const noexcept
    {
        return _checkpoints.eventRecordCount();
    }

    const boost::optional<PacketDecodingError>& error() const noexcept
    {
        return _checkpoints.error();
    }

    void eventRecordsAtIndexInPacket(Index indexInPacket, Size count,
                                     std::vector<EventRecord::SP>& eventRecords);
    const EventRecord& eventRecordAtIndexInPacket(Index indexInPacket);
    boost::optional<Index> eventRecordIndexInPacketAtOffsetInPacketBits(Index offsetInPacketBits);

    /*
    void eventRecordsByOffset(Index offsetInPacketBits, Size count,
                              std::vector<EventRecord::SP>& eventRecords);
    void dataRegions(Index offsetInPacketBits, Size sizeBits,
                     std::vector<DataRegion::SP>& dataRegions);
    */

private:
    boost::optional<Index> _dataRegionIndexInCache(Index offsetInPacketBits);
    boost::optional<Index> _eventRecordIndexInCacheByOffsetInPacketBits(Index offsetInPacketBits);

    Index _eventRecordIndexInCacheFromIndexInPacket(const Index indexInPacket)
    {
        const auto baseIndex = this->_eventRecordBaseIndexInCacheFromIndexInPacket(indexInPacket);
        return indexInPacket - baseIndex;
    }

    void _cacheEventRecordsAtIndexInPacket(const Index reqIndexInPacket)
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

    void _cacheEventRecordsFromCurIt(Index indexInPacket)
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

    Index _eventRecordBaseIndexInCacheFromIndexInPacket(const Index indexInPacket)
    {
        assert(indexInPacket < _checkpoints.eventRecordCount());
        return indexInPacket - (indexInPacket % _eventRecordCacheMaxSize);
    }

private:
    const PacketIndexEntry *_indexEntry;
    yactfr::PacketSequence *_seq;
    const Metadata *_metadata;
    yactfr::DataSource::UP _dataSrc;
    yactfr::PacketSequenceIterator _it;
    yactfr::PacketSequenceIterator _endIt;
    PacketCheckpoints _checkpoints;
    std::vector<DataRegion::SP> _dataRegionCache;
    std::vector<EventRecord::SP> _eventRecordCache;
    Size _eventRecordCacheMaxSize;
    yactfr::PacketSequenceIteratorPosition _posAfterEventRecordCache;
};

} // namespace jacques

#endif // _JACQUES_PACKET_HPP
