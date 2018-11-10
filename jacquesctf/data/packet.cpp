/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <yactfr/metadata/string-type.hpp>
#include <yactfr/metadata/struct-type.hpp>

#include "packet.hpp"
#include "content-packet-region.hpp"
#include "padding-packet-region.hpp"
#include "error-packet-region.hpp"
#include "logging.hpp"

namespace jacques {

Packet::Packet(const PacketIndexEntry& indexEntry,
               yactfr::PacketSequence& seq, const Metadata& metadata,
               yactfr::DataSource::UP dataSrc,
               std::unique_ptr<MemoryMappedFile> mmapFile,
               PacketCheckpointsBuildListener& packetCheckpointsBuildListener) :
    _indexEntry {&indexEntry},
    _metadata {&metadata},
    _dataSrc {std::move(dataSrc)},
    _mmapFile {std::move(mmapFile)},
    _it {std::begin(seq)},
    _endIt {std::end(seq)},
    _checkpoints {
        seq, metadata, *_indexEntry, 20011, packetCheckpointsBuildListener,
    },
    _lruRegionCache {2000},
    _preambleSize {
        indexEntry.preambleSize() ? *indexEntry.preambleSize() :
        indexEntry.effectiveContentSize()
    }
{
    _mmapFile->map(_indexEntry->offsetInDataStreamBytes(),
                   _indexEntry->effectiveTotalSize());
    theLogger->debug("Packet's memory mapped file: path `{}`, address 0x{:x}, "
                     "offset {}, size {} B, file size {} B.",
                     _mmapFile->path().string(),
                     reinterpret_cast<std::uintptr_t>(_mmapFile->addr()),
                     _mmapFile->offsetBytes(), _mmapFile->size().bytes(),
                     _mmapFile->fileSize().bytes());
    this->_cachePreambleRegions();
}

void Packet::_ensureEventRecordIsCached(const Index indexInPacket)
{
    assert(indexInPacket < _checkpoints.eventRecordCount());

    theLogger->debug("Ensuring event record #{} is cached.",
                     indexInPacket);

    if (this->_eventRecordIsCached(indexInPacket)) {
        theLogger->debug("Event record #{} is already cached.",
                         indexInPacket);
        return;
    }

    const auto halfMaxCacheSize = _eventRecordCacheMaxSize / 2;
    const auto toCacheIndexInPacket = indexInPacket < halfMaxCacheSize ? 0 :
                                      indexInPacket - halfMaxCacheSize;

    // find nearest event record checkpoint
    const auto cp = _checkpoints.nearestCheckpointBeforeOrAtIndex(toCacheIndexInPacket);

    assert(cp);

    auto curIndex = cp->first->indexInPacket();

    theLogger->debug("Found nearest checkpoint for event record #{}: #{}.",
                     indexInPacket, curIndex);
    _it.restorePosition(cp->second);

    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (curIndex == toCacheIndexInPacket) {
                const auto count = std::min(_eventRecordCacheMaxSize,
                                            _checkpoints.eventRecordCount() - curIndex);

                this->_cacheRegionsFromErsAtCurIt(curIndex, count);
                return;
            }

            ++curIndex;
        }

        ++_it;
    }
}

void Packet::_ensureOffsetInPacketBitsIsCached(const Index offsetInPacketBits)
{
    if (this->_regionCacheContainsOffsetInPacketBits(_regionCache,
                                                     offsetInPacketBits)) {
        return;
    }

    // preamble?
    if (this->_regionCacheContainsOffsetInPacketBits(_preambleRegionCache,
                                                     offsetInPacketBits)) {
        // this is the current cache now
        _regionCache = _preambleRegionCache;
        _eventRecordCache.clear();
        return;
    }

    assert(_checkpoints.eventRecordCount() > 0);

    const auto& lastEventRecord = *_checkpoints.lastEventRecord();

    if (offsetInPacketBits >= lastEventRecord.segment().offsetInPacketBits()) {
        // last event record or after
        this->_ensureEventRecordIsCached(lastEventRecord.indexInPacket());
        return;
    }

    // find nearest event record checkpoint by offset
    const auto cp = _checkpoints.nearestCheckpointBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

    assert(cp);

    auto curIndex = cp->first->indexInPacket();

    _it.restorePosition(cp->second);

    // find closest event record before or containing offset
    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (this->_itOffsetInPacketBits() == offsetInPacketBits) {
                break;
            } else if (this->_itOffsetInPacketBits() > offsetInPacketBits) {
                // we want the previous one which includes `offsetInPacketBits`
                assert(curIndex != 0);
                --curIndex;
                break;
            }

        } else if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_END) {
            ++curIndex;
        }

        ++_it;
    }

    // no we have its index: cache event records around this one
    this->_ensureEventRecordIsCached(curIndex);

    /*
     * This scenario can happen:
     *
     * 1. _ensureEventRecordIsCached() above did not do anything because
     *    the event record at `curIndex` is already in cache.
     * 2. The event record at `curIndex` is the event record cache's
     *    last one.
     * 3. `offsetInPacketBits` is a padding region between two event
     *    records.
     * 4. `offsetInPacketBits` is greater than or equal to the cache's
     *    last event record's end offset.
     *
     * Just in case, make sure that the following event record is also
     * in cache. If it was not, then the caches will be cleared and many
     * event records will be cached around the one at `curIndex`,
     * therefore the packet region cache will include the padding
     * region.
     */
    ++curIndex;

    if (curIndex < _checkpoints.eventRecordCount()) {
        this->_ensureEventRecordIsCached(curIndex);
    }
}

void Packet::_cacheContentRegionAtCurIt(Scope::SP scope)
{
    using ElemKind = yactfr::Element::Kind;

    PacketRegion::SP region;

    switch (_it->kind()) {
    case ElemKind::SIGNED_INT:
    case ElemKind::SIGNED_ENUM:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::SignedIntElement>(scope);
        break;

    case ElemKind::UNSIGNED_INT:
    case ElemKind::UNSIGNED_ENUM:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::UnsignedIntElement>(scope);
        break;

    case ElemKind::FLOAT:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FloatElement>(scope);
        break;

    case ElemKind::STRING_BEGINNING:
    case ElemKind::STATIC_TEXT_ARRAY_BEGINNING:
    case ElemKind::DYNAMIC_TEXT_ARRAY_BEGINNING:
    {
        // strings are always aligned within the packet
        assert(this->_itOffsetInPacketBits() % 8 == 0);

        // get appropriate data type
        const yactfr::DataType *type;

        switch (_it->kind()) {
        case ElemKind::STRING_BEGINNING:
            type = &static_cast<const yactfr::StringBeginningElement&>(*_it).type();
            break;

        case ElemKind::STATIC_TEXT_ARRAY_BEGINNING:
            type = &static_cast<const yactfr::StaticTextArrayBeginningElement&>(*_it).type();
            break;

        case ElemKind::DYNAMIC_TEXT_ARRAY_BEGINNING:
            type = &static_cast<const yactfr::DynamicTextArrayBeginningElement&>(*_it).type();
            break;
        default:
            std::abort();
        }

        assert(type);

        const auto offsetStartBits = this->_itOffsetInPacketBits();
        const auto bufStart = _mmapFile->addr() + this->_itOffsetInPacketBytes();
        auto bufEnd = bufStart;

        ++_it;

        while (_it->kind() != ElemKind::STRING_END &&
                _it->kind() != ElemKind::STATIC_TEXT_ARRAY_END &&
                _it->kind() != ElemKind::DYNAMIC_TEXT_ARRAY_END) {
            assert(_it->kind() == ElemKind::SUBSTRING);

            // "consume" this substring
            bufEnd += static_cast<const yactfr::SubstringElement&>(*_it).size();
            ++_it;
        }

        /*
         * Find end of string in buffer. std::find() returns either
         * the location of the (first) null character or `bufEnd`.
         */
        const auto bufStrEnd = std::find(bufStart, bufEnd, 0);

        // create string value
        std::string str {
            reinterpret_cast<const char *>(bufStart),
            static_cast<std::string::size_type>(bufStrEnd - bufStart)
        };

        const PacketSegment segment {
            offsetStartBits,
            DataSize::fromBytes(bufEnd - bufStart)
        };

        // okay to move the scope here, it's never used afterwards
        region = std::make_shared<ContentPacketRegion>(segment,
                                                       std::move(scope),
                                                       *type,
                                                       ContentPacketRegion::Value {str});
        break;
    }

    default:
        break;
    }

    // caller expects the iterator to be passed this packet region
    ++_it;

    assert(region);
    this->_trySetPreviousRegionOffsetInPacketBits(*region);
    _regionCache.push_back(std::move(region));
}

void Packet::_tryCachePaddingRegionBeforeCurIt(Scope::SP scope)
{
    PacketSegment segment;

    if (_regionCache.empty()) {
        if (this->_itOffsetInPacketBits() == 0 ||
                this->_itOffsetInPacketBits() >= _preambleSize) {
            return;
        }

        segment = PacketSegment {0, this->_itOffsetInPacketBits()};
    } else {
        const auto& prevRegion = _regionCache.back();

        if (*prevRegion->segment().endOffsetInPacketBits() ==
                this->_itOffsetInPacketBits()) {
            return;
        }

        assert(*prevRegion->segment().endOffsetInPacketBits() <
               this->_itOffsetInPacketBits());

        segment = PacketSegment {
            *prevRegion->segment().endOffsetInPacketBits(),
            this->_itOffsetInPacketBits() -
            *prevRegion->segment().endOffsetInPacketBits(),
            prevRegion->segment().byteOrder()
        };
    }

    auto region = std::make_shared<PaddingPacketRegion>(segment,
                                                              std::move(scope));

    this->_trySetPreviousRegionOffsetInPacketBits(*region);
    _regionCache.push_back(std::move(region));
}

void Packet::_cachePreambleRegions()
{
    theLogger->debug("Caching preamble packet regions.");

    using ElemKind = yactfr::Element::Kind;

    assert(_preambleRegionCache.empty());
    assert(_regionCache.empty());

    // go to beginning of packet
    theLogger->debug("Seeking packet at offset {} B.",
                     _indexEntry->offsetInDataStreamBytes());
    _it.seekPacket(_indexEntry->offsetInDataStreamBytes());

    // special case: no event records and an error: cache everything now
    if (_checkpoints.error() && _checkpoints.eventRecordCount() == 0) {
        this->_cacheRegionsAtCurItUntilError(0);
        _preambleRegionCache = _regionCache;
        return;
    }

    Scope::SP curScope;
    bool isDone = false;

    try {
        while (!isDone) {
            // TODO: replace with element visitor
            switch (_it->kind()) {
            case ElemKind::SIGNED_INT:
            case ElemKind::UNSIGNED_INT:
            case ElemKind::SIGNED_ENUM:
            case ElemKind::UNSIGNED_ENUM:
            case ElemKind::FLOAT:
            case ElemKind::STRING_BEGINNING:
            case ElemKind::STATIC_TEXT_ARRAY_BEGINNING:
            case ElemKind::DYNAMIC_TEXT_ARRAY_BEGINNING:
                this->_tryCachePaddingRegionBeforeCurIt(curScope);

                // _cacheContentRegionAtCurIt() increments the iterator
                this->_cacheContentRegionAtCurIt(curScope);
                break;

            case ElemKind::SCOPE_BEGINNING:
            {
                // cache padding before scope
                this->_tryCachePaddingRegionBeforeCurIt(curScope);

                auto& elem = static_cast<const yactfr::ScopeBeginningElement&>(*_it);

                curScope = std::make_shared<Scope>(elem.scope());
                curScope->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
                ++_it;
                break;
            }

            case ElemKind::STRUCT_BEGINNING:
            {
                if (curScope && !curScope->dataType()) {
                    auto& elem = static_cast<const yactfr::StructBeginningElement&>(*_it);

                    curScope->dataType(elem.type());
                }

                ++_it;
                break;
            }

            case ElemKind::SCOPE_END:
            {
                assert(curScope);
                curScope->segment().size(this->_itOffsetInPacketBits() -
                                         curScope->segment().offsetInPacketBits());
                curScope = nullptr;
                ++_it;
                break;
            }

            case ElemKind::EVENT_RECORD_BEGINNING:
                // cache padding before first event record
                this->_tryCachePaddingRegionBeforeCurIt(curScope);
                isDone = true;
                break;

            case ElemKind::PACKET_CONTENT_END:
                // cache padding before end of packet
                while (_it->kind() != ElemKind::PACKET_END) {
                    ++_it;
                }

                this->_tryCachePaddingRegionBeforeCurIt(curScope);
                isDone = true;
                break;

            default:
                ++_it;
                break;
            }
        }
    } catch (const yactfr::DecodingError&) {
        theLogger->debug("Got a decoding error at offset {} b: "
                         "appending a preamble packet region.",
                         _it.offset());

        Index offsetStartBits = 0;
        OptByteOrder byteOrder;

        // remaining data until end of packet is an error region
        if (!_regionCache.empty()) {
            offsetStartBits = *_regionCache.back()->segment().endOffsetInPacketBits();
            byteOrder = _regionCache.back()->segment().byteOrder();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalSize().bits();

        if (offsetEndBits != offsetStartBits) {
            const PacketSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits, byteOrder
            };
            auto region = std::make_shared<ErrorPacketRegion>(segment);

            this->_trySetPreviousRegionOffsetInPacketBits(*region);
            _regionCache.push_back(std::move(region));
        }
    }

    if (!_regionCache.empty()) {
        theLogger->debug("Preamble packet region cache now spans [{} b, {} b[.",
                         _regionCache.front()->segment().offsetInPacketBits(),
                         _regionCache.back()->segment().offsetInPacketBits() +
                         _regionCache.back()->segment().size()->bits());
    }

    _preambleRegionCache = _regionCache;
}

void Packet::_cacheRegionsAtCurIt(const yactfr::Element::Kind endElemKind,
                                  Index erIndexInPacket)
{
    using ElemKind = yactfr::Element::Kind;

    EventRecord::SP curEr;
    Scope::SP curScope;
    bool isDone = false;

    while (!isDone) {
        if (_it->kind() == endElemKind) {
            // done after this iteration
            isDone = true;
        }

        // TODO: replace with element visitor
        switch (_it->kind()) {
        case ElemKind::SIGNED_INT:
        case ElemKind::UNSIGNED_INT:
        case ElemKind::SIGNED_ENUM:
        case ElemKind::UNSIGNED_ENUM:
        case ElemKind::FLOAT:
        case ElemKind::STRING_BEGINNING:
        case ElemKind::STATIC_TEXT_ARRAY_BEGINNING:
        case ElemKind::DYNAMIC_TEXT_ARRAY_BEGINNING:
            this->_tryCachePaddingRegionBeforeCurIt(curScope);

            // _cacheContentRegionAtCurIt() increments the iterator
            this->_cacheContentRegionAtCurIt(curScope);
            break;

        case ElemKind::SCOPE_BEGINNING:
        {
            // cache padding before scope
            this->_tryCachePaddingRegionBeforeCurIt(curScope);

            auto& elem = static_cast<const yactfr::ScopeBeginningElement&>(*_it);

            curScope = std::make_shared<Scope>(curEr, elem.scope());
            curScope->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
            ++_it;
            break;
        }

        case ElemKind::STRUCT_BEGINNING:
        {
            if (curScope && !curScope->dataType()) {
                auto& elem = static_cast<const yactfr::StructBeginningElement&>(*_it);

                curScope->dataType(elem.type());
            }

            ++_it;
            break;
        }

        case ElemKind::SCOPE_END:
            if (curScope) {
                curScope->segment().size(this->_itOffsetInPacketBits() -
                                         curScope->segment().offsetInPacketBits());
                curScope = nullptr;
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_BEGINNING:
            // cache padding before event record
            this->_tryCachePaddingRegionBeforeCurIt(curScope);
            curEr = std::make_shared<EventRecord>(erIndexInPacket);
            curEr->segment().offsetInPacketBits(this->_itOffsetInPacketBits());

            // immediately cache it because this loop could throw before the end
            _eventRecordCache.push_back(curEr);
            ++_it;
            break;

        case ElemKind::EVENT_RECORD_END:
            if (curEr) {
                curEr->segment().size(this->_itOffsetInPacketBits() -
                                      curEr->segment().offsetInPacketBits());
                curScope = nullptr;
                curEr = nullptr;
                ++erIndexInPacket;
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_TYPE:
            if (curEr) {
                auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*_it);

                curEr->type(elem.eventRecordType());
            }

            ++_it;
            break;

        case ElemKind::CLOCK_VALUE:
            if (curEr) {
                if (!curEr->firstTimestamp() && _metadata->isCorrelatable()) {
                    auto& elem = static_cast<const yactfr::ClockValueElement&>(*_it);

                    curEr->firstTimestamp(Timestamp {elem});
                }
            }

            ++_it;
            break;

        default:
            ++_it;
            break;
        }
    }
}

void Packet::_cacheRegionsFromOneErAtCurIt(const Index indexInPacket)
{
    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    this->_cacheRegionsAtCurIt(ElemKind::EVENT_RECORD_END, indexInPacket);
}

void Packet::_cacheRegionsAtCurItUntilError(const Index initErIndexInPacket)
{
    try {
        this->_cacheRegionsAtCurIt(yactfr::Element::Kind::PACKET_END,
                                   initErIndexInPacket);
    } catch (const yactfr::DecodingError&) {
        Index offsetStartBits = _preambleSize.bits();
        OptByteOrder byteOrder;

        // remaining data until end of packet is an error region
        if (!_regionCache.empty()) {
            offsetStartBits = *_regionCache.back()->segment().endOffsetInPacketBits();
            byteOrder = _regionCache.back()->segment().byteOrder();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalSize().bits();

        if (offsetEndBits != offsetStartBits) {
            const PacketSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits, byteOrder
            };
            auto region = std::make_shared<ErrorPacketRegion>(segment);

            this->_trySetPreviousRegionOffsetInPacketBits(*region);
            _regionCache.push_back(std::move(region));
        }
    }
}

void Packet::_cacheRegionsFromErsAtCurIt(const Index erIndexInPacket,
                                         const Size erCount)
{
    /*
     * This function's logic:
     *
     *     Cache all requested event records, minus one if the packet
     *       has an error and it's the last event record. This step does
     *       not throw a decoding error: this would be a bug.
     *
     *     If we need to cache the last event record:
     *         If the packet has an error:
     *             Cache all regions until said error. This will
     *               necessarily create the last event record, but it
     *               could be incomplete.
     *         Else:
     *             Cache any padding region after the last event record.
     */
    assert(erCount > 0);

    theLogger->debug("Caching event records #{} to #{}.",
                     erIndexInPacket, erIndexInPacket + erCount - 1);

    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    _regionCache.clear();
    _eventRecordCache.clear();

    const auto endErIndexInPacket = erIndexInPacket + erCount;
    auto endErIndexInPacketBeforeLast = endErIndexInPacket;

    if (_checkpoints.error() &&
            endErIndexInPacketBeforeLast == _checkpoints.eventRecordCount()) {
        --endErIndexInPacketBeforeLast;
    }

    assert(erIndexInPacket <= endErIndexInPacketBeforeLast);

    for (Index index = erIndexInPacket;
            index < endErIndexInPacketBeforeLast; ++index) {
        while (_it->kind() != ElemKind::EVENT_RECORD_BEGINNING) {
            assert(_it->kind() != ElemKind::PACKET_CONTENT_END);
            ++_it;
        }

        this->_cacheRegionsFromOneErAtCurIt(index);
    }

    if (endErIndexInPacket == _checkpoints.eventRecordCount()) {
        if (_checkpoints.error()) {
            /*
             * This last event record might not contain the last data
             * because there's a decoding error in the packet. Continue
             * caching packet regions until we reach this error, and
             * then create an error packet region with the remaining
             * data.
             */
            this->_cacheRegionsAtCurItUntilError(endErIndexInPacketBeforeLast);
        } else {
            // end of packet: also cache any padding before the end of packet
            while (_it->kind() != ElemKind::PACKET_END) {
                ++_it;
            }

            this->_tryCachePaddingRegionBeforeCurIt(nullptr);
        }
    }

    if (!_regionCache.empty()) {
        theLogger->debug("Packet region cache now spans [{} b, {} b[.",
                         _regionCache.front()->segment().offsetInPacketBits(),
                         _regionCache.back()->segment().offsetInPacketBits() +
                         _regionCache.back()->segment().size()->bits());
    }
}

const PacketRegion& Packet::regionAtOffsetInPacketBits(const Index offsetInPacketBits)
{
    theLogger->debug("Requesting single packet region at offset {} b.",
                     offsetInPacketBits);

    auto regionFromLru = _lruRegionCache.get(offsetInPacketBits);

    if (regionFromLru) {
        theLogger->debug("LRU cache hit: cache size {}.",
                         _lruRegionCache.size());
        return **regionFromLru;
    }

    theLogger->debug("LRU cache miss: cache size {}.",
                     _lruRegionCache.size());
    this->_ensureOffsetInPacketBitsIsCached(offsetInPacketBits);

    const auto it = this->_regionCacheItBeforeOrAtOffsetInPacketBits(offsetInPacketBits);
    const auto& region = **it;

    /*
     * Add both the requested offset and the actual packet region's offset
     * to the cache so that future requests using this exact offset hit
     * the cache.
     */
    if (!_lruRegionCache.contains(offsetInPacketBits)) {
        theLogger->debug("Adding to LRU cache (offset {} b).",
                         offsetInPacketBits);
        _lruRegionCache.insert(offsetInPacketBits, *it);
    }

    const auto drOffsetInPacketBits = region.segment().offsetInPacketBits();

    if (!_lruRegionCache.contains(drOffsetInPacketBits)) {
        theLogger->debug("Adding to LRU cache (offset {} b).",
                         drOffsetInPacketBits);
        _lruRegionCache.insert(drOffsetInPacketBits, *it);
    }

    return region;
}

const PacketRegion& Packet::lastRegion()
{
    // request the packet's last bit: then we know we have the last packet region
    return this->regionAtOffsetInPacketBits(_indexEntry->effectiveTotalSize().bits() - 1);
}

const PacketRegion& Packet::firstRegion()
{
    // request the packet's last bit: then we know we have the last packet region
    return this->regionAtOffsetInPacketBits(0);
}

const PacketRegion *Packet::previousRegion(const PacketRegion& region)
{
    // previous
    if (region.segment().offsetInPacketBits() == 0) {
        return nullptr;
    }

    if (region.previousRegionOffsetInPacketBits()) {
        return &this->regionAtOffsetInPacketBits(*region.previousRegionOffsetInPacketBits());
    }

    return &this->regionAtOffsetInPacketBits(region.segment().offsetInPacketBits() - 1);
}

const EventRecord *Packet::eventRecordBeforeOrAtNsFromOrigin(const long long nsFromOrigin)
{
    const auto cpNearestFunc = [this](const long long nsFromOrigin) -> const PacketCheckpoints::Checkpoint * {
        return _checkpoints.nearestCheckpointBeforeOrAtNsFromOrigin(nsFromOrigin);
    };
    const auto getPropFunc = [](const Timestamp& ts) -> long long {
        return ts.nsFromOrigin();
    };

    return this->_eventRecordBeforeOrAtTs(cpNearestFunc, getPropFunc, nsFromOrigin);
}

const EventRecord *Packet::eventRecordBeforeOrAtCycles(const unsigned long long cycles)
{
    const auto cpNearestFunc = [this](const unsigned long long cycles) -> const PacketCheckpoints::Checkpoint * {
        return _checkpoints.nearestCheckpointBeforeOrAtCycles(cycles);
    };
    const auto getPropFunc = [](const Timestamp& ts) -> unsigned long long {
        return ts.cycles();
    };

    return this->_eventRecordBeforeOrAtTs(cpNearestFunc, getPropFunc, cycles);
}

} // namespace jacques
