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
#include "logging.hpp"

namespace jacques {

Packet::Packet(const PacketIndexEntry& indexEntry,
               yactfr::PacketSequence& seq,
               const Metadata& metadata,
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
    }
{
    _mmapFile->map(_indexEntry->offsetInDataStreamBytes(),
                   _indexEntry->totalSize());
    theLogger->debug("Packet's memory mapped file: path `{}`, address 0x{:x}, "
                     "offset {}, size {} B, file size {} B.",
                     _mmapFile->path().string(),
                     reinterpret_cast<std::uintptr_t>(_mmapFile->addr()),
                     _mmapFile->offsetBytes(), _mmapFile->size().bytes(),
                     _mmapFile->fileSize().bytes());
}

void Packet::_ensureEventRecordIsCached(const Index indexInPacket)
{
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
    if (this->_dataRegionCacheContainsOffsetInPacketBits(offsetInPacketBits)) {
        return;
    }

    // find nearest event record checkpoint by offset
    auto cp = _checkpoints.nearestCheckpointBeforeOrAtOffsetInPacketBits(offsetInPacketBits);

    assert(cp);

    auto curIndex = cp->first->indexInPacket();

    _it.restorePosition(cp->second);

    // find closest event record before or containing offset
    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (this->_itOffsetInPacketBits() >= offsetInPacketBits) {
                break;
            }

            ++curIndex;
        }

        ++_it;
    }

    // no we have its index: cache event records around this one
    this->_ensureEventRecordIsCached(curIndex);
}

void Packet::_cacheContentDataRegionAtCurIt(Scope::SP scope)
{
    assert(scope);

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

        // copy data in buffer to region's data vector
        DataRegion::Data data;

        std::transform(bufStart, bufEnd, std::back_inserter(data),
                       [](const auto ch) {
            return static_cast<std::uint8_t>(ch);
        });

        /*
         * Find end of string in buffer. std::find() returns either
         * the location of the null character or `bufEnd`.
         */
        const auto bufStrEnd = std::find(bufStart, bufEnd, 0);

        // create string value
        std::string str;

        std::transform(bufStart, bufStrEnd, std::back_inserter(str),
                       [](const auto byte) {
            return static_cast<char>(byte);
        });

        const DataSegment segment {offsetStartBits, data.size() * 8};

        // okay to move the scope here, it's never used afterwards
        region = std::make_shared<ContentDataRegion>(segment,
                                                     std::move(data),
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
    _dataRegionCache.push_back(std::move(region));
}

void Packet::_tryCachePaddingDataRegionBeforeCurIt(Scope::SP scope)
{
    if (!_dataRegionCache.empty()) {
        const auto& lastDataRegion = _dataRegionCache.back();

        if (lastDataRegion->segment().endOffsetInPacketBits() != this->_itOffsetInPacketBits()) {
            DataRegion::Data data;
            DataSegment segment {
                lastDataRegion->segment().endOffsetInPacketBits(),
                this->_itOffsetInPacketBits() - lastDataRegion->segment().endOffsetInPacketBits()
            };

            this->_fillDataBytesForSegment(data, segment);

            auto dataRegion = std::make_shared<PaddingDataRegion>(segment,
                                                                  std::move(data),
                                                                  std::move(scope),
                                                                  lastDataRegion->byteOrder());

            _dataRegionCache.push_back(std::move(dataRegion));
        }
    }
}

void Packet::_cachePacketPreambleDataRegions()
{
    theLogger->debug("Caching preamble data regions.");

    using ElemKind = yactfr::Element::Kind;

    // clear current cache
    _dataRegionCache.clear();
    _eventRecordCache.clear();

    // go to beginning of packet
    theLogger->debug("Seeking packet at offset {} B.",
                     _indexEntry->offsetInDataStreamBytes());
    _it.seekPacket(_indexEntry->offsetInDataStreamBytes());

    Scope::SP curScope;
    bool isDone = false;

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

    theLogger->debug("Data region cache now spans [{} b, {} b[.",
                     _dataRegionCache.front()->segment().offsetInPacketBits(),
                     _dataRegionCache.back()->segment().offsetInPacketBits() +
                     _dataRegionCache.back()->segment().size().bits());
}

void Packet::_cacheDataRegionsFromOneErAtCurIt(const Index indexInPacket)
{
    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);

    EventRecord::SP curEr;
    Scope::SP curScope;

    while (true) {
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

            assert(curEr);
            curScope = std::make_shared<Scope>(curEr, elem.scope());
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
        {
            curEr = std::make_shared<EventRecord>(indexInPacket);
            curEr->segment().offsetInPacketBits(this->_itOffsetInPacketBits());
            ++_it;
            break;
        }

        case ElemKind::EVENT_RECORD_END:
        {
            assert(curEr);
            curEr->segment().size(this->_itOffsetInPacketBits() -
                                  curEr->segment().offsetInPacketBits());
            _eventRecordCache.push_back(std::move(curEr));
            return;
        }

        case ElemKind::EVENT_RECORD_TYPE:
        {
            auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*_it);

            assert(curEr);
            curEr->type(elem.eventRecordType());
            ++_it;
            break;
        }

        case ElemKind::CLOCK_VALUE:
        {
            assert(curEr);

            if (curEr->firstTimestamp() || !_metadata->isCorrelatable()) {
                ++_it;
                break;
            }

            auto& elem = static_cast<const yactfr::ClockValueElement&>(*_it);

            curEr->firstTimestamp(Timestamp {elem});
            ++_it;
            break;
        }

        default:
            ++_it;
            break;
        }
    }
}

void Packet::_cacheDataRegionsFromErsAtCurIt(const Index erIndexInPacket,
                                             const Size erCount)
{
    theLogger->debug("Caching event records #{} to #{}.",
                     erIndexInPacket, erIndexInPacket + erCount - 1);

    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    _dataRegionCache.clear();
    _eventRecordCache.clear();

    const auto endErIndexInPacket = erIndexInPacket + erCount;

    for (Index index = erIndexInPacket; index < endErIndexInPacket; ++index) {
        while (_it->kind() != ElemKind::EVENT_RECORD_BEGINNING) {
            assert (_it->kind() != ElemKind::PACKET_CONTENT_END);
            ++_it;
        }

        this->_cacheDataRegionsFromOneErAtCurIt(index);
    }

    if (endErIndexInPacket == _checkpoints.eventRecordCount()) {
        // end of packet: also cache any padding before the end of packet
        while (_it->kind() != ElemKind::PACKET_END) {
            ++_it;
        }

        this->_tryCachePaddingDataRegionBeforeCurIt(nullptr);
    }

    theLogger->debug("Data region cache now spans [{} b, {} b[.",
                     _dataRegionCache.front()->segment().offsetInPacketBits(),
                     _dataRegionCache.back()->segment().offsetInPacketBits() +
                     _dataRegionCache.back()->segment().size().bits());
}

void Packet::appendDataRegionsAtOffsetInPacketBits(std::vector<DataRegion::SP>& regions,
                                                   Index offsetInPacketBits,
                                                   Index endOffsetInPacketBits)
{
    theLogger->debug("Appending data regions for user in [{} b, {} b[.",
                     offsetInPacketBits, endOffsetInPacketBits);
    assert(offsetInPacketBits < _indexEntry->totalSize().bits());
    assert(endOffsetInPacketBits <= _indexEntry->totalSize().bits());
    assert(offsetInPacketBits < endOffsetInPacketBits);

    DataSize preambleSize;

    if (_checkpoints.eventRecordCount() > 0) {
        // preamble is until beginning of first event record
        preambleSize = _checkpoints.firstEventRecord()->segment().offsetInPacketBits();
    } else {
        // preamble is the only content
        preambleSize = _indexEntry->contentSize();
    }

    theLogger->debug("Preamble size: {} b.", preambleSize.bits());

    Index curOffsetInPacketBits;

    // append preamble regions if needed
    if (offsetInPacketBits < preambleSize.bits()) {
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

} // namespace jacques
