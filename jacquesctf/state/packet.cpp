/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <yactfr/metadata/string-type.hpp>

#include "packet.hpp"
#include "content-data-region.hpp"
#include "padding-data-region.hpp"
#include "error-data-region.hpp"
#include "data-stream-file-state.hpp"
#include "state.hpp"
#include "logging.hpp"

namespace jacques {

Packet::Packet(DataStreamFileState& dsfState,
               const PacketIndexEntry& indexEntry,
               yactfr::PacketSequence& seq,
               const Metadata& metadata,
               yactfr::DataSource::UP dataSrc,
               std::unique_ptr<MemoryMappedFile> mmapFile,
               PacketCheckpointsBuildListener& packetCheckpointsBuildListener) :
    _state {&dsfState.state()},
    _indexEntry {&indexEntry},
    _metadata {&metadata},
    _dataSrc {std::move(dataSrc)},
    _mmapFile {std::move(mmapFile)},
    _it {std::begin(seq)},
    _endIt {std::end(seq)},
    _checkpoints {
        seq, metadata, *_indexEntry, 20011, packetCheckpointsBuildListener,
    },
    _lruDataRegionCache {2000},
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
    this->_cachePacketPreambleDataRegions();
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
    auto cp = _checkpoints.nearestCheckpointBeforeOrAtIndex(toCacheIndexInPacket);

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

                this->_cacheDataRegionsFromErsAtCurIt(curIndex, count);
                return;
            }

            ++curIndex;
        }

        ++_it;
    }
}

void Packet::_ensureOffsetInPacketBitsIsCached(const Index offsetInPacketBits)
{
    if (this->_dataRegionCacheContainsOffsetInPacketBits(_dataRegionCache,
                                                         offsetInPacketBits)) {
        return;
    }

    // preamble?
    if (this->_dataRegionCacheContainsOffsetInPacketBits(_preambleDataRegionCache,
                                                         offsetInPacketBits)) {
        // this is the current cache now
        _dataRegionCache = _preambleDataRegionCache;
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
    auto cp = _checkpoints.nearestCheckpointBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

    if (!cp) {
        /*
         * There's not even a single checkpoint. The only way this can
         * happen is if the first event record is not decodable (there's
         * a decoding error). Let's just cache the whole packet in this
         * case (preamble + partial first event record + error).
         */
        assert(_checkpoints.error());
        _it.seekPacket(_indexEntry->offsetInDataStreamBytes());
        this->_cacheDataRegionsAtCurItUntilError();
        return;
    }

    assert(cp);

    auto nextIndex = cp->first->indexInPacket();
    Index index;

    _it.restorePosition(cp->second);

    // find closest event record before or containing offset
    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (this->_itOffsetInPacketBits() >= offsetInPacketBits) {
                index = nextIndex;
                break;
            }

            ++nextIndex;
        }

        ++_it;
    }

    // no we have its index: cache event records around this one
    this->_ensureEventRecordIsCached(index);
}

void Packet::_cacheContentDataRegionAtCurIt(Scope::SP scope)
{
    using ElemKind = yactfr::Element::Kind;

    DataRegion::SP region;

    switch (_it->kind()) {
    case ElemKind::SIGNED_INT:
    case ElemKind::SIGNED_ENUM:
        region = this->_contentDataRegionFromBitArrayElemAtCurIt<yactfr::SignedIntElement>(scope);
        break;

    case ElemKind::UNSIGNED_INT:
    case ElemKind::UNSIGNED_ENUM:
        region = this->_contentDataRegionFromBitArrayElemAtCurIt<yactfr::UnsignedIntElement>(scope);
        break;

    case ElemKind::FLOAT:
        region = this->_contentDataRegionFromBitArrayElemAtCurIt<yactfr::FloatElement>(scope);
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

        const DataRegion::DataRange dataRange {
            reinterpret_cast<const std::uint8_t *>(bufStart),
            reinterpret_cast<const std::uint8_t *>(bufEnd)
        };

        /*
         * Find end of string in buffer. std::find() returns either
         * the location of the null character or `bufEnd`.
         */
        const auto bufStrEnd = std::find(bufStart, bufEnd, 0);

        // create string value
        std::string str {
            reinterpret_cast<const char *>(bufStart),
            static_cast<std::string::size_type>(bufStrEnd - bufStart)
        };

        const DataSegment segment {offsetStartBits, (bufEnd - bufStart) * 8};

        // okay to move the scope here, it's never used afterwards
        region = std::make_shared<ContentDataRegion>(segment,
                                                     dataRange,
                                                     std::move(scope),
                                                     *type,
                                                     ContentDataRegion::Value {str});
        break;
    }

    default:
        break;
    }

    // caller expects the iterator to be passed this data region
    ++_it;

    assert(region);
    this->_trySetPreviousDataRegionOffsetInPacketBits(*region);
    _dataRegionCache.push_back(std::move(region));
}

void Packet::_tryCachePaddingDataRegionBeforeCurIt(Scope::SP scope)
{
    DataSegment segment;
    boost::optional<ByteOrder> byteOrder;

    if (_dataRegionCache.empty()) {
        if (this->_itOffsetInPacketBits() == 0 ||
                this->_itOffsetInPacketBits() >= _preambleSize.bits()) {
            return;
        }

        segment = DataSegment {0, this->_itOffsetInPacketBits()};
    } else {
        const auto& prevDataRegion = _dataRegionCache.back();

        if (prevDataRegion->segment().endOffsetInPacketBits() ==
                this->_itOffsetInPacketBits()) {
            return;
        }

        assert(prevDataRegion->segment().endOffsetInPacketBits() <
               this->_itOffsetInPacketBits());

        segment = DataSegment {
            prevDataRegion->segment().endOffsetInPacketBits(),
            this->_itOffsetInPacketBits() -
            prevDataRegion->segment().endOffsetInPacketBits()
        };
        byteOrder = prevDataRegion->byteOrder();
    }

    const auto dataRange = this->_dataRangeForSegment(segment);
    auto dataRegion = std::make_shared<PaddingDataRegion>(segment,
                                                          dataRange,
                                                          std::move(scope),
                                                          byteOrder);

    this->_trySetPreviousDataRegionOffsetInPacketBits(*dataRegion);
    _dataRegionCache.push_back(std::move(dataRegion));
}

void Packet::_cachePacketPreambleDataRegions()
{
    theLogger->debug("Caching preamble data regions.");

    using ElemKind = yactfr::Element::Kind;

    assert(_preambleDataRegionCache.empty());
    assert(_dataRegionCache.empty());

    // go to beginning of packet
    theLogger->debug("Seeking packet at offset {} B.",
                     _indexEntry->offsetInDataStreamBytes());
    _it.seekPacket(_indexEntry->offsetInDataStreamBytes());

    // special case: no event records and an error: cache everything now
    if (_checkpoints.error() && _checkpoints.eventRecordCount() == 0) {
        this->_cacheDataRegionsAtCurItUntilError();
        _preambleDataRegionCache = _dataRegionCache;
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
                this->_tryCachePaddingDataRegionBeforeCurIt(curScope);

                // _cacheContentDataRegionAtCurIt() increments the iterator
                this->_cacheContentDataRegionAtCurIt(curScope);
                break;

            case ElemKind::SCOPE_BEGINNING:
            {
                auto& elem = static_cast<const yactfr::ScopeBeginningElement&>(*_it);

                curScope = std::make_shared<Scope>(elem.scope());
                curScope->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
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
                this->_tryCachePaddingDataRegionBeforeCurIt(curScope);
                isDone = true;
                break;

            case ElemKind::PACKET_CONTENT_END:
                // cache padding before end of packet
                while (_it->kind() != ElemKind::PACKET_END) {
                    ++_it;
                }

                this->_tryCachePaddingDataRegionBeforeCurIt(curScope);
                isDone = true;
                break;

            default:
                ++_it;
                break;
            }
        }
    } catch (const yactfr::DecodingError&) {
        theLogger->debug("Got a decoding error at offset {} b: "
                         "appending a preamble data region.",
                         _it.offset());

        Index offsetStartBits = 0;
        boost::optional<ByteOrder> byteOrder;

        // remaining data until end of packet is an error region
        if (!_dataRegionCache.empty()) {
            offsetStartBits = _dataRegionCache.back()->segment().endOffsetInPacketBits();
            byteOrder = _dataRegionCache.back()->byteOrder();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalSize().bits();

        if (offsetEndBits != offsetStartBits) {
            const DataSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits
            };
            const auto dataRange = this->_dataRangeForSegment(segment);
            auto dataRegion = std::make_shared<ErrorDataRegion>(segment,
                                                                dataRange,
                                                                byteOrder);

            this->_trySetPreviousDataRegionOffsetInPacketBits(*dataRegion);
            _dataRegionCache.push_back(std::move(dataRegion));
        }
    }

    if (!_dataRegionCache.empty()) {
        theLogger->debug("Preamble data region cache now spans [{} b, {} b[.",
                         _dataRegionCache.front()->segment().offsetInPacketBits(),
                         _dataRegionCache.back()->segment().offsetInPacketBits() +
                         _dataRegionCache.back()->segment().size().bits());
    }

    _preambleDataRegionCache = _dataRegionCache;
}

void Packet::_cacheDataRegionsAtCurIt(const yactfr::Element::Kind endElemKind,
                                      const bool setCurScope,
                                      const bool setCurEventRecord,
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
            this->_tryCachePaddingDataRegionBeforeCurIt(curScope);

            // _cacheContentDataRegionAtCurIt() increments the iterator
            this->_cacheContentDataRegionAtCurIt(curScope);
            break;

        case ElemKind::SCOPE_BEGINNING:
            if (setCurScope) {
                auto& elem = static_cast<const yactfr::ScopeBeginningElement&>(*_it);

                assert(curEr);
                curScope = std::make_shared<Scope>(curEr, elem.scope());
                curScope->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
            }

            ++_it;
            break;

        case ElemKind::SCOPE_END:
            if (setCurScope && curScope) {
                curScope->segment().size(this->_itOffsetInPacketBits() -
                                         curScope->segment().offsetInPacketBits());
                curScope = nullptr;
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_BEGINNING:
            if (setCurEventRecord) {
                curEr = std::make_shared<EventRecord>(erIndexInPacket);
                curEr->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_END:
            if (setCurEventRecord && curEr) {
                curEr->segment().size(this->_itOffsetInPacketBits() -
                                      curEr->segment().offsetInPacketBits());
                _eventRecordCache.push_back(std::move(curEr));
                curEr = nullptr;
                curScope = nullptr;
                ++erIndexInPacket;
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_TYPE:
            if (setCurEventRecord && curEr) {
                auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*_it);

                curEr->type(elem.eventRecordType());
            }

            ++_it;
            break;

        case ElemKind::CLOCK_VALUE:
            if (setCurEventRecord && curEr) {
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

void Packet::_cacheDataRegionsFromOneErAtCurIt(const Index indexInPacket)
{
    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    this->_cacheDataRegionsAtCurIt(yactfr::Element::Kind::EVENT_RECORD_END,
                                   true, true, indexInPacket);
}

void Packet::_cacheDataRegionsAtCurItUntilError()
{
    try {
        this->_cacheDataRegionsAtCurIt(yactfr::Element::Kind::PACKET_END,
                                       false, false, 0);
    } catch (const yactfr::DecodingError&) {
        Index offsetStartBits = _preambleSize.bits();
        boost::optional<ByteOrder> byteOrder;

        // remaining data until end of packet is an error region
        if (!_dataRegionCache.empty()) {
            offsetStartBits = _dataRegionCache.back()->segment().endOffsetInPacketBits();
            byteOrder = _dataRegionCache.back()->byteOrder();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalSize().bits();

        if (offsetEndBits != offsetStartBits) {
            const DataSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits
            };
            const auto dataRange = this->_dataRangeForSegment(segment);
            auto dataRegion = std::make_shared<ErrorDataRegion>(segment,
                                                                dataRange,
                                                                byteOrder);

            this->_trySetPreviousDataRegionOffsetInPacketBits(*dataRegion);
            _dataRegionCache.push_back(std::move(dataRegion));
        }
    }
}

void Packet::_cacheDataRegionsFromErsAtCurIt(const Index erIndexInPacket,
                                             const Size erCount)
{
    assert(erCount > 0);

    theLogger->debug("Caching event records #{} to #{}.",
                     erIndexInPacket, erIndexInPacket + erCount - 1);

    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    _dataRegionCache.clear();
    _eventRecordCache.clear();

    const auto endErIndexInPacket = erIndexInPacket + erCount;

    for (Index index = erIndexInPacket; index < endErIndexInPacket; ++index) {
        while (_it->kind() != ElemKind::EVENT_RECORD_BEGINNING) {
            assert(_it->kind() != ElemKind::PACKET_CONTENT_END);
            ++_it;
        }

        this->_cacheDataRegionsFromOneErAtCurIt(index);
    }

    if (endErIndexInPacket == _checkpoints.eventRecordCount()) {
        if (_checkpoints.error()) {
            /*
             * This last event record might not contain the last data
             * because there's a decoding error in the packet. Continue
             * caching data regions until we reach this error, and then
             * create an error data region with the remaining data.
             */
            this->_cacheDataRegionsAtCurItUntilError();
        } else {
            // end of packet: also cache any padding before the end of packet
            while (_it->kind() != ElemKind::PACKET_END) {
                ++_it;
            }

            this->_tryCachePaddingDataRegionBeforeCurIt(nullptr);
        }
    }

    if (!_dataRegionCache.empty()) {
        theLogger->debug("Data region cache now spans [{} b, {} b[.",
                         _dataRegionCache.front()->segment().offsetInPacketBits(),
                         _dataRegionCache.back()->segment().offsetInPacketBits() +
                         _dataRegionCache.back()->segment().size().bits());
    }
}

void Packet::appendDataRegions(std::vector<DataRegion::SP>& regions,
                               const Index offsetInPacketBits,
                               const Index endOffsetInPacketBits)
{
    theLogger->debug("Appending data regions for user in [{} b, {} b[.",
                     offsetInPacketBits, endOffsetInPacketBits);
    assert(offsetInPacketBits < _indexEntry->effectiveTotalSize().bits());
    assert(endOffsetInPacketBits <= _indexEntry->effectiveTotalSize().bits());
    assert(offsetInPacketBits < endOffsetInPacketBits);
    theLogger->debug("Preamble size: {} b.", _preambleSize.bits());

    Index curOffsetInPacketBits;

    // append preamble regions if needed
    if (offsetInPacketBits < _preambleSize.bits()) {
        this->_cachePacketPreambleDataRegions();

        auto it = this->_dataRegionCacheItBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

        curOffsetInPacketBits = (*it)->segment().offsetInPacketBits();
        theLogger->debug("Starting offset (inside preamble): {} b.",
                         curOffsetInPacketBits);

        while (true) {
            if (it == std::end(_dataRegionCache)) {
                break;
            }

            const auto& regionOffset = (*it)->segment().offsetInPacketBits();
            const auto& regionSize = (*it)->segment().size();

            regions.push_back(*it);
            ++it;

            curOffsetInPacketBits = regionOffset + regionSize.bits();

            if (curOffsetInPacketBits >= endOffsetInPacketBits) {
                return;
            }
        }
    } else {
        curOffsetInPacketBits = offsetInPacketBits;
        theLogger->debug("Starting offset (outside preamble): {} b.",
                         curOffsetInPacketBits);
    }

    if (curOffsetInPacketBits >= endOffsetInPacketBits) {
        return;
    }

    while (true) {
        this->_ensureOffsetInPacketBitsIsCached(curOffsetInPacketBits);

        auto it = this->_dataRegionCacheItBeforeOrAtOffsetInPacketBits(curOffsetInPacketBits);

        /*
         * If `curOffsetInPacketBits` was the exact offset of a data
         * region, then it is unchanged here.
         */
        curOffsetInPacketBits = (*it)->segment().offsetInPacketBits();
        theLogger->debug("Current offset: {} b.", curOffsetInPacketBits);

        while (true) {
            if (it == std::end(_dataRegionCache)) {
                // need to cache more
                theLogger->debug("End of current data region cache: "
                                 "cache more (current offset: {} b).",
                                 curOffsetInPacketBits);
                break;
            }

            const auto& regionOffset = (*it)->segment().offsetInPacketBits();
            const auto& regionSize = (*it)->segment().size();

            regions.push_back(*it);
            ++it;

            curOffsetInPacketBits = regionOffset + regionSize.bits();

            if (curOffsetInPacketBits >= endOffsetInPacketBits) {
                return;
            }
        }
    }
}

const DataRegion& Packet::dataRegionAtOffsetInPacketBits(const Index offsetInPacketBits)
{
    theLogger->debug("Requesting single data region at offset {} b.",
                     offsetInPacketBits);

    auto dataRegionFromLru = _lruDataRegionCache.get(offsetInPacketBits);

    if (dataRegionFromLru) {
        theLogger->debug("LRU cache hit: cache size {}.",
                         _lruDataRegionCache.size());
        return **dataRegionFromLru;
    }

    theLogger->debug("LRU cache miss: cache size {}.",
                     _lruDataRegionCache.size());
    this->_ensureOffsetInPacketBitsIsCached(offsetInPacketBits);

    const auto it = this->_dataRegionCacheItBeforeOrAtOffsetInPacketBits(offsetInPacketBits);
    const auto& dataRegion = **it;

    /*
     * Add both the requested offset and the actual data region's offset
     * to the cache so that future requests using this exact offset hit
     * the cache.
     */
    if (!_lruDataRegionCache.contains(offsetInPacketBits)) {
        theLogger->debug("Adding to LRU cache (offset {} b).",
                         offsetInPacketBits);
        _lruDataRegionCache.insert(offsetInPacketBits, *it);
    }

    const auto drOffsetInPacketBits = dataRegion.segment().offsetInPacketBits();

    if (!_lruDataRegionCache.contains(drOffsetInPacketBits)) {
        theLogger->debug("Adding to LRU cache (offset {} b).",
                         drOffsetInPacketBits);
        _lruDataRegionCache.insert(drOffsetInPacketBits, *it);
    }

    return dataRegion;
}

void Packet::curOffsetInPacketBits(const Index offsetInPacketBits)
{
    if (offsetInPacketBits == _curOffsetInPacketBits) {
        return;
    }

    assert(offsetInPacketBits < _indexEntry->effectiveTotalSize().bits());
    _curOffsetInPacketBits = offsetInPacketBits;
    _state->_notify(CurOffsetInPacketChangedMessage {});
}

const DataRegion& Packet::lastDataRegion()
{
    // request the packet's last bit: then we know we have the last data region
    return this->dataRegionAtOffsetInPacketBits(_indexEntry->effectiveTotalSize().bits() - 1);
}

const DataRegion& Packet::firstDataRegion()
{
    // request the packet's last bit: then we know we have the last data region
    return this->dataRegionAtOffsetInPacketBits(0);
}

} // namespace jacques
