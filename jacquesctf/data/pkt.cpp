/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <yactfr/metadata/string-type.hpp>
#include <yactfr/metadata/struct-type.hpp>

#include "pkt.hpp"
#include "content-pkt-region.hpp"
#include "padding-pkt-region.hpp"
#include "error-pkt-region.hpp"

namespace jacques {

Pkt::Pkt(const PktIndexEntry& indexEntry, yactfr::ElementSequence& seq, const Metadata& metadata,
         yactfr::DataSource::UP dataSrc, std::unique_ptr<MemMappedFile> mmapFile,
         PktCheckpointsBuildListener& pktCheckpointsBuildListener) :
    _indexEntry {&indexEntry},
    _metadata {&metadata},
    _dataSrc {std::move(dataSrc)},
    _mmapFile {std::move(mmapFile)},
    _it {seq.begin()},
    _endIt {seq.end()},
    _checkpoints {
        seq, metadata, *_indexEntry, 20011, pktCheckpointsBuildListener,
    },
    _lruRegionCache {2000},
    _preambleLen {
        indexEntry.preambleLen() ? *indexEntry.preambleLen() : indexEntry.effectiveContentLen()
    }
{
    _mmapFile->map(_indexEntry->offsetInDsFileBytes(), _indexEntry->effectiveTotalLen());
    this->_cachePreambleRegions();
}

void Pkt::_ensureErIsCached(const Index indexInPkt)
{
    assert(indexInPkt < _checkpoints.erCount());

    // current cache?
    if (this->_erIsCached(_curErCache, indexInPkt)) {
        return;
    }

    // last cache?
    if (this->_erIsCached(_lastErCache, indexInPkt)) {
        // this is the current cache now
        _curRegionCache = std::move(_lastRegionCache);
        _curErCache = std::move(_lastErCache);
        return;
    }

    const auto halfMaxCacheSize = _erCacheMaxSize / 2;
    const auto toCacheIndexInPkt = indexInPkt < halfMaxCacheSize ? 0 : indexInPkt - halfMaxCacheSize;

    // find nearest event record checkpoint
    const auto cp = _checkpoints.nearestCheckpointBeforeOrAtIndex(toCacheIndexInPkt);

    assert(cp);

    auto curIndex = cp->first->indexInPkt();

    _it.restorePosition(cp->second);

    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (curIndex == toCacheIndexInPkt) {
                const auto count = std::min(_erCacheMaxSize, _checkpoints.erCount() - curIndex);

                this->_cacheRegionsFromErsAtCurIt(curIndex, count);
                return;
            }

            ++curIndex;
        }

        ++_it;
    }
}

void Pkt::_ensureOffsetInPktBitsIsCached(const Index offsetInPktBits)
{
    // current region cache?
    if (this->_regionCacheContainsOffsetInPktBits(_curRegionCache, offsetInPktBits)) {
        return;
    }

    // preamble region cache?
    if (this->_regionCacheContainsOffsetInPktBits(_preambleRegionCache, offsetInPktBits)) {
        // this is the current cache now
        _lastRegionCache = std::move(_curRegionCache);
        _lastErCache = std::move(_curErCache);
        _curRegionCache = _preambleRegionCache;
        _curErCache.clear();
        return;
    }

    // last region cache?
    if (this->_regionCacheContainsOffsetInPktBits(_lastRegionCache, offsetInPktBits)) {
        // this is the current cache now
        _curRegionCache = std::move(_lastRegionCache);
        _curErCache = std::move(_lastErCache);
        return;
    }

    assert(_checkpoints.erCount() > 0);

    const auto& lastEr = *_checkpoints.lastEr();

    if (offsetInPktBits >= lastEr.segment().offsetInPktBits()) {
        // last event record or after
        this->_ensureErIsCached(lastEr.indexInPkt());
        return;
    }

    // find nearest event record checkpoint by offset
    const auto cp = _checkpoints.nearestCheckpointBeforeOrAtOffsetInPktBits(offsetInPktBits);

    assert(cp);

    auto curIndex = cp->first->indexInPkt();

    _it.restorePosition(cp->second);

    // find closest event record before or containing offset
    while (true) {
        if (_it->kind() == yactfr::Element::Kind::EVENT_RECORD_BEGINNING) {
            if (this->_itOffsetInPktBits() == offsetInPktBits) {
                break;
            } else if (this->_itOffsetInPktBits() > offsetInPktBits) {
                // we want the previous one which includes `offsetInPktBits`
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
    this->_ensureErIsCached(curIndex);

    /*
     * This scenario can happen:
     *
     * 1. _ensureErIsCached() above did not do anything because
     *    the event record at `curIndex` is already in cache.
     *
     * 2. The event record at `curIndex` is the last one of the event
     *    record cache.
     *
     * 3. `offsetInPktBits` is a padding region between two event
     *    records.
     *
     * 4. `offsetInPktBits` is greater than or equal to the end offset
     *    of the last event record of the cache.
     *
     * Just in case, make sure that the following event record is also
     * in cache. If it wasn't, then the caches will be cleared and many
     * event records will be cached around the one at `curIndex`,
     * therefore the packet region cache will include the padding
     * region.
     */
    ++curIndex;

    if (curIndex < _checkpoints.erCount()) {
        this->_ensureErIsCached(curIndex);
    }
}

void Pkt::_cacheContentRegionAtCurIt(Scope::SP scope)
{
    using ElemKind = yactfr::Element::Kind;

    PktRegion::SP region;

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
        assert(this->_itOffsetInPktBits() % 8 == 0);

        // get appropriate data type
        const yactfr::DataType *dt;

        switch (_it->kind()) {
        case ElemKind::STRING_BEGINNING:
            dt = &static_cast<const yactfr::StringBeginningElement&>(*_it).type();
            break;

        case ElemKind::STATIC_TEXT_ARRAY_BEGINNING:
            dt = &static_cast<const yactfr::StaticTextArrayBeginningElement&>(*_it).type();
            break;

        case ElemKind::DYNAMIC_TEXT_ARRAY_BEGINNING:
            dt = &static_cast<const yactfr::DynamicTextArrayBeginningElement&>(*_it).type();
            break;
        default:
            std::abort();
        }

        assert(dt);

        const auto offsetStartBits = this->_itOffsetInPktBits();
        const auto bufStart = _mmapFile->addr() + this->_itOffsetInPktBytes();
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

        const PktSegment segment {
            offsetStartBits,
            DataLen::fromBytes(bufEnd - bufStart)
        };

        // okay to move the scope here, it's never used afterwards
        region = std::make_shared<ContentPktRegion>(segment, std::move(scope), *dt,
                                                    ContentPktRegion::Val {str});
        break;
    }

    default:
        break;
    }

    assert(region);
    this->_trySetPreviousRegionOffsetInPktBits(*region);
    _curRegionCache.push_back(std::move(region));

    /*
     * Caller expects the iterator to be passed this packet region. Do
     * it after caching the region because `++_it` could throw a
     * decoding error.
     */
    ++_it;
}

void Pkt::_tryCachePaddingRegionBeforeCurIt(Scope::SP scope)
{
    PktSegment segment;

    if (_curRegionCache.empty()) {
        if (this->_itOffsetInPktBits() == 0 || this->_itOffsetInPktBits() >= _preambleLen) {
            return;
        }

        segment = PktSegment {0, this->_itOffsetInPktBits()};
    } else {
        const auto& prevRegion = _curRegionCache.back();

        if (*prevRegion->segment().endOffsetInPktBits() == this->_itOffsetInPktBits()) {
            return;
        }

        assert(*prevRegion->segment().endOffsetInPktBits() < this->_itOffsetInPktBits());

        segment = PktSegment {
            *prevRegion->segment().endOffsetInPktBits(),
            this->_itOffsetInPktBits() -
            *prevRegion->segment().endOffsetInPktBits(),
            prevRegion->segment().bo()
        };
    }

    auto region = std::make_shared<PaddingPktRegion>(segment, std::move(scope));

    this->_trySetPreviousRegionOffsetInPktBits(*region);
    _curRegionCache.push_back(std::move(region));
}

void Pkt::_cachePreambleRegions()
{
    using ElemKind = yactfr::Element::Kind;

    assert(_preambleRegionCache.empty());
    assert(_curRegionCache.empty());

    // go to beginning of packet
    _it.seekPacket(_indexEntry->offsetInDsFileBytes());

    // special case: no event records and an error: cache everything now
    if (_checkpoints.error() && _checkpoints.erCount() == 0) {
        this->_cacheRegionsAtCurItUntilError(0);
        _preambleRegionCache = std::move(_curRegionCache);
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
                curScope->segment().offsetInPktBits(this->_itOffsetInPktBits());
                ++_it;
                break;
            }

            case ElemKind::STRUCT_BEGINNING:
            {
                if (curScope && !curScope->dt()) {
                    auto& elem = static_cast<const yactfr::StructBeginningElement&>(*_it);

                    curScope->dt(elem.type());
                }

                ++_it;
                break;
            }

            case ElemKind::SCOPE_END:
            {
                assert(curScope);
                curScope->segment().len(this->_itOffsetInPktBits() -
                                        curScope->segment().offsetInPktBits());
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
        Index offsetStartBits = 0;
        OptBo bo;

        // remaining data until end of packet is an error region
        if (!_curRegionCache.empty()) {
            offsetStartBits = *_curRegionCache.back()->segment().endOffsetInPktBits();
            bo = _curRegionCache.back()->segment().bo();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalLen().bits();

        if (offsetEndBits != offsetStartBits) {
            const PktSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits, bo
            };
            auto region = std::make_shared<ErrorPktRegion>(segment);

            this->_trySetPreviousRegionOffsetInPktBits(*region);
            _curRegionCache.push_back(std::move(region));
        }
    }

    _preambleRegionCache = std::move(_curRegionCache);
}

void Pkt::_cacheRegionsAtCurIt(const yactfr::Element::Kind endElemKind, Index erIndexInPkt)
{
    using ElemKind = yactfr::Element::Kind;

    Er::SP curEr;
    Scope::SP curScope;
    auto isDone = false;

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
            curScope->segment().offsetInPktBits(this->_itOffsetInPktBits());
            ++_it;
            break;
        }

        case ElemKind::STRUCT_BEGINNING:
        {
            if (curScope && !curScope->dt()) {
                auto& elem = static_cast<const yactfr::StructBeginningElement&>(*_it);

                curScope->dt(elem.type());
            }

            ++_it;
            break;
        }

        case ElemKind::SCOPE_END:
            if (curScope) {
                curScope->segment().len(this->_itOffsetInPktBits() -
                                        curScope->segment().offsetInPktBits());
                curScope = nullptr;
            }

            ++_it;
            break;

        case ElemKind::EVENT_RECORD_BEGINNING:
            // cache padding before event record
            this->_tryCachePaddingRegionBeforeCurIt(curScope);
            curEr = std::make_shared<Er>(erIndexInPkt);
            curEr->segment().offsetInPktBits(this->_itOffsetInPktBits());

            // immediately cache it because this loop could throw before the end
            _curErCache.push_back(curEr);
            ++_it;
            break;

        case ElemKind::EVENT_RECORD_END:
            if (curEr) {
                curEr->segment().len(this->_itOffsetInPktBits() -
                                    curEr->segment().offsetInPktBits());
                curScope = nullptr;
                curEr = nullptr;
                ++erIndexInPkt;
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
                if (!curEr->firstTs() && _metadata->isCorrelatable()) {
                    auto& elem = static_cast<const yactfr::ClockValueElement&>(*_it);

                    curEr->firstTs(Ts {elem});
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

void Pkt::_cacheRegionsFromOneErAtCurIt(const Index indexInPkt)
{
    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    this->_cacheRegionsAtCurIt(ElemKind::EVENT_RECORD_END, indexInPkt);
}

void Pkt::_cacheRegionsAtCurItUntilError(const Index initErIndexInPkt)
{
    try {
        this->_cacheRegionsAtCurIt(yactfr::Element::Kind::PACKET_END, initErIndexInPkt);
    } catch (const yactfr::DecodingError&) {
        Index offsetStartBits = _preambleLen.bits();
        OptBo bo;

        // remaining data until end of packet is an error region
        if (!_curRegionCache.empty()) {
            offsetStartBits = *_curRegionCache.back()->segment().endOffsetInPktBits();
            bo = _curRegionCache.back()->segment().bo();
        }

        const auto offsetEndBits = _indexEntry->effectiveTotalLen().bits();

        if (offsetEndBits != offsetStartBits) {
            const PktSegment segment {
                offsetStartBits, offsetEndBits - offsetStartBits, bo
            };
            auto region = std::make_shared<ErrorPktRegion>(segment);

            this->_trySetPreviousRegionOffsetInPktBits(*region);
            _curRegionCache.push_back(std::move(region));
        }
    }
}

void Pkt::_cacheRegionsFromErsAtCurIt(const Index erIndexInPkt, const Size erCount)
{
    /*
     * The logic of this function:
     *
     *     Cache all requested event records, minus one if the packet
     *     has an error and it's the last event record. This step does
     *     not throw a decoding error: this would be a bug.
     *
     *     If we need to cache the last event record:
     *         If the packet has an error:
     *             Cache all regions until said error. This will
     *             necessarily create the last event record, but it
     *             could be incomplete.
     *         Else:
     *             Cache any padding region after the last event record.
     */
    assert(erCount > 0);

    using ElemKind = yactfr::Element::Kind;

    assert(_it->kind() == ElemKind::EVENT_RECORD_BEGINNING);
    _curRegionCache.clear();
    _curErCache.clear();

    const auto endErIndexInPkt = erIndexInPkt + erCount;
    auto endErIndexInPktBeforeLast = endErIndexInPkt;

    if (_checkpoints.error() && endErIndexInPktBeforeLast == _checkpoints.erCount()) {
        --endErIndexInPktBeforeLast;
    }

    assert(erIndexInPkt <= endErIndexInPktBeforeLast);

    for (auto index = erIndexInPkt; index < endErIndexInPktBeforeLast; ++index) {
        while (_it->kind() != ElemKind::EVENT_RECORD_BEGINNING) {
            assert(_it->kind() != ElemKind::PACKET_CONTENT_END);
            ++_it;
        }

        this->_cacheRegionsFromOneErAtCurIt(index);
    }

    if (endErIndexInPkt == _checkpoints.erCount()) {
        if (_checkpoints.error()) {
            /*
             * This last event record might not contain the last data
             * because there's a decoding error in the packet. Continue
             * caching packet regions until we reach this error, and
             * then create an error packet region with the remaining
             * data.
             */
            this->_cacheRegionsAtCurItUntilError(endErIndexInPktBeforeLast);
        } else {
            // end of packet: also cache any padding before the end of packet
            while (_it->kind() != ElemKind::PACKET_END) {
                ++_it;
            }

            this->_tryCachePaddingRegionBeforeCurIt(nullptr);
        }
    }
}

const PktRegion& Pkt::regionAtOffsetInPktBits(const Index offsetInPktBits)
{
    auto regionFromLru = _lruRegionCache.get(offsetInPktBits);

    if (regionFromLru) {
        return **regionFromLru;
    }

    this->_ensureOffsetInPktBitsIsCached(offsetInPktBits);

    const auto it = this->_regionCacheItBeforeOrAtOffsetInPktBits(offsetInPktBits);
    const auto& region = **it;

    /*
     * Add both the requested offset and the actual offset of the packet
     * region to the cache so that future requests using this exact
     * offset hit the cache.
     */
    if (!_lruRegionCache.contains(offsetInPktBits)) {
        _lruRegionCache.insert(offsetInPktBits, *it);
    }

    const auto drOffsetInPktBits = region.segment().offsetInPktBits();

    if (!_lruRegionCache.contains(drOffsetInPktBits)) {
        _lruRegionCache.insert(drOffsetInPktBits, *it);
    }

    return region;
}

const PktRegion& Pkt::lastRegion()
{
    /*
     * Request the last bit of the packet: then we know we have the last
     * packet region.
     */
    return this->regionAtOffsetInPktBits(_indexEntry->effectiveTotalLen().bits() - 1);
}

const PktRegion& Pkt::firstRegion()
{
    /*
     * Request the last bit of the packet: then we know we have the last
     * packet region.
     */
    return this->regionAtOffsetInPktBits(0);
}

const PktRegion *Pkt::previousRegion(const PktRegion& region)
{
    // previous
    if (region.segment().offsetInPktBits() == 0) {
        return nullptr;
    }

    if (region.previousRegionOffsetInPktBits()) {
        return &this->regionAtOffsetInPktBits(*region.previousRegionOffsetInPktBits());
    }

    return &this->regionAtOffsetInPktBits(region.segment().offsetInPktBits() - 1);
}

const Er *Pkt::erBeforeOrAtNsFromOrigin(const long long nsFromOrigin)
{
    const auto cpNearestFunc = [this](const long long nsFromOrigin) -> const PktCheckpoints::Checkpoint * {
        return _checkpoints.nearestCheckpointBeforeOrAtNsFromOrigin(nsFromOrigin);
    };

    const auto getPropFunc = [](const Ts& ts) -> long long {
        return ts.nsFromOrigin();
    };

    return this->_erBeforeOrAtTs(cpNearestFunc, getPropFunc, nsFromOrigin);
}

const Er *Pkt::erBeforeOrAtCycles(const unsigned long long cycles)
{
    const auto cpNearestFunc = [this](const unsigned long long cycles) -> const PktCheckpoints::Checkpoint * {
        return _checkpoints.nearestCheckpointBeforeOrAtCycles(cycles);
    };

    const auto getPropFunc = [](const Ts& ts) -> unsigned long long {
        return ts.cycles();
    };

    return this->_erBeforeOrAtTs(cpNearestFunc, getPropFunc, cycles);
}

} // namespace jacques
