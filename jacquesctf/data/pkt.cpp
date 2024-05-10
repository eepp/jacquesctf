/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <yactfr/yactfr.hpp>

#include "pkt.hpp"
#include "content-pkt-region.hpp"
#include "padding-pkt-region.hpp"
#include "error-pkt-region.hpp"
#include "utils.hpp"

namespace jacques {

Pkt::Pkt(const PktIndexEntry& indexEntry, yactfr::ElementSequence& seq, const Metadata& metadata,
         yactfr::DataSource::Up dataSrc, std::unique_ptr<MemMappedFile> mmapFile,
         PktCheckpointsBuildListener& pktCheckpointsBuildListener) :
    _indexEntry {&indexEntry},
    _metadata {&metadata},
    _dataSrc {std::move(dataSrc)},
    _mmapFile {std::move(mmapFile)},
    _it {seq.begin()},
    _endIt {seq.end()},
    _checkpoints {
        seq, metadata, *_indexEntry, 3779, pktCheckpointsBuildListener,
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
        if (_it->isEventRecordBeginningElement()) {
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
        if (_it->isEventRecordBeginningElement()) {
            if (this->_itOffsetInPktBits() == offsetInPktBits) {
                break;
            } else if (this->_itOffsetInPktBits() > offsetInPktBits) {
                // we want the previous one which includes `offsetInPktBits`
                assert(curIndex != 0);
                --curIndex;
                break;
            }
        } else if (_it->isEventRecordEndElement()) {
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
    case ElemKind::FixedLengthBitArray:
    {
        const auto val = _it->asFixedLengthBitArrayElement().unsignedIntegerValue();

        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthBitArrayElement>(scope,
                                                                                                 val);
        break;
    }

    case ElemKind::FixedLengthBitMap:
    {
        const auto val = _it->asFixedLengthBitMapElement().unsignedIntegerValue();

        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthBitMapElement>(scope,
                                                                                               val);
        break;
    }

    case ElemKind::FixedLengthBoolean:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthBooleanElement>(scope);
        break;

    case ElemKind::FixedLengthSignedInteger:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthSignedIntegerElement>(scope);
        break;

    case ElemKind::FixedLengthUnsignedInteger:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthUnsignedIntegerElement>(scope);
        break;

    case ElemKind::FixedLengthFloatingPointNumber:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::FixedLengthFloatingPointNumberElement>(scope);
        break;

    case ElemKind::VariableLengthSignedInteger:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::VariableLengthSignedIntegerElement>(scope);
        break;

    case ElemKind::VariableLengthUnsignedInteger:
        region = this->_contentRegionFromBitArrayElemAtCurIt<yactfr::VariableLengthUnsignedIntegerElement>(scope);
        break;

    case ElemKind::NullTerminatedStringBeginning:
    case ElemKind::StaticLengthStringBeginning:
    case ElemKind::DynamicLengthStringBeginning:
    {
        // null-terminated strings are always byte-aligned within the packet
        assert(this->_itOffsetInPktBits() % 8 == 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
        // get corresponding data type
        auto& dt = utils::call([this]() -> const yactfr::DataType& {
            if (_it->isNullTerminatedStringBeginningElement()) {
                return _it->asNullTerminatedStringBeginningElement().type();
            } else if (_it->isStaticLengthStringBeginningElement()) {
                return _it->asStaticLengthStringBeginningElement().type();
            } else {
                assert(_it->isDynamicLengthStringBeginningElement());
                return _it->asDynamicLengthStringBeginningElement().type();
            }
        });
#pragma GCC diagnostic pop

        const auto offsetStartBits = this->_itOffsetInPktBits();
        const auto bufStart = _mmapFile->addr() + this->_itOffsetInPktBytes();
        auto bufEnd = bufStart;

        ++_it;

        while (!_it->isNullTerminatedStringEndElement() &&
                !_it->isStaticLengthStringEndElement() &&
                !_it->isDynamicLengthStringEndElement()) {
            assert(_it->isRawDataElement());

            // "consume" this substring
            bufEnd += _it->asRawDataElement().size();
            ++_it;
        }

        auto str = utils::call([&dt, &bufStart, &bufEnd] {
            if (dt.isStringType() &&
                    dt.asStringType().encoding() == yactfr::StringEncoding::Utf8) {
                /*
                 * Find end of string in buffer. std::find() returns
                 * either the location of the (first) null character or
                 * `bufEnd`.
                 */
                const auto bufStrEnd = std::find(bufStart, bufEnd, 0);

                // create string value
                return std::string {
                    reinterpret_cast<const char *>(bufStart),
                    static_cast<std::string::size_type>(bufStrEnd - bufStart)
                };
            } else {
                // TODO: decode value, remove this temporary message
                return std::string {"(not an UTF-8 string)"};
            }
        });

        const PktSegment segment {
            offsetStartBits,
            DataLen::fromBytes(bufEnd - bufStart)
        };

        region = std::make_shared<ContentPktRegion>(segment, std::move(scope), dt,
                                                    ContentPktRegion::Val {std::move(str)});
        break;
    }

    case ElemKind::StaticLengthBlobBeginning:
    case ElemKind::DynamicLengthBlobBeginning:
    {
        // BLOBs are always byte-aligned within the packet
        assert(this->_itOffsetInPktBits() % 8 == 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
        // get corresponding data type
        auto& dt = utils::call([this]() -> const yactfr::DataType& {
            if (_it->isStaticLengthBlobBeginningElement()) {
                return _it->asStaticLengthBlobBeginningElement().type();
            } else {
                assert(_it->isDynamicLengthBlobBeginningElement());
                return _it->asDynamicLengthBlobBeginningElement().type();
            }
        });
#pragma GCC diagnostic pop

        const auto offsetStartBits = this->_itOffsetInPktBits();
        const auto bufStart = _mmapFile->addr() + this->_itOffsetInPktBytes();
        auto bufEnd = bufStart;

        ++_it;

        while (!_it->isStaticLengthBlobEndElement() && !_it->isDynamicLengthBlobEndElement()) {
            if (_it->isRawDataElement()) {
                // "consume" this BLOB section
                bufEnd += _it->asRawDataElement().size();
            }

            ++_it;
        }

        const PktSegment segment {
            offsetStartBits,
            DataLen::fromBytes(bufEnd - bufStart)
        };

        region = std::make_shared<ContentPktRegion>(segment, std::move(scope), dt,
                                                    ContentPktRegion::Val {nullptr});
        break;
    }

    default:
        break;
    }

    assert(region);
    this->_trySetPrevRegionOffsetInPktBits(*region);
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

    this->_trySetPrevRegionOffsetInPktBits(*region);
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
            case ElemKind::FixedLengthBitArray:
            case ElemKind::FixedLengthBitMap:
            case ElemKind::FixedLengthBoolean:
            case ElemKind::FixedLengthSignedInteger:
            case ElemKind::FixedLengthUnsignedInteger:
            case ElemKind::FixedLengthFloatingPointNumber:
            case ElemKind::VariableLengthSignedInteger:
            case ElemKind::VariableLengthUnsignedInteger:
            case ElemKind::NullTerminatedStringBeginning:
            case ElemKind::StaticLengthStringBeginning:
            case ElemKind::DynamicLengthStringBeginning:
            case ElemKind::StaticLengthBlobBeginning:
            case ElemKind::DynamicLengthBlobBeginning:
                this->_tryCachePaddingRegionBeforeCurIt(curScope);

                // _cacheContentRegionAtCurIt() increments the iterator
                this->_cacheContentRegionAtCurIt(curScope);
                break;

            case ElemKind::ScopeBeginning:
            {
                // cache padding before scope
                this->_tryCachePaddingRegionBeforeCurIt(curScope);

                curScope = std::make_shared<Scope>(_it->asScopeBeginningElement().scope());
                curScope->segment().offsetInPktBits(this->_itOffsetInPktBits());
                ++_it;
                break;
            }

            case ElemKind::StructureBeginning:
            {
                if (curScope && !curScope->dt()) {
                    curScope->dt(_it->asStructureBeginningElement().type());
                }

                ++_it;
                break;
            }

            case ElemKind::ScopeEnd:
            {
                assert(curScope);
                curScope->segment().len(this->_itOffsetInPktBits() -
                                        curScope->segment().offsetInPktBits());
                curScope = nullptr;
                ++_it;
                break;
            }

            case ElemKind::EventRecordBeginning:
                // cache padding before first event record
                this->_tryCachePaddingRegionBeforeCurIt(curScope);
                isDone = true;
                break;

            case ElemKind::PacketContentEnd:
                // cache padding before end of packet
                while (!_it->isPacketEndElement()) {
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
            auto region = std::make_shared<ErrorPktRegion>(PktSegment {
                offsetStartBits, offsetEndBits - offsetStartBits, bo
            });

            this->_trySetPrevRegionOffsetInPktBits(*region);
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
        case ElemKind::FixedLengthBitArray:
        case ElemKind::FixedLengthBitMap:
        case ElemKind::FixedLengthBoolean:
        case ElemKind::FixedLengthSignedInteger:
        case ElemKind::FixedLengthUnsignedInteger:
        case ElemKind::FixedLengthFloatingPointNumber:
        case ElemKind::VariableLengthSignedInteger:
        case ElemKind::VariableLengthUnsignedInteger:
        case ElemKind::NullTerminatedStringBeginning:
        case ElemKind::StaticLengthStringBeginning:
        case ElemKind::DynamicLengthStringBeginning:
        case ElemKind::StaticLengthBlobBeginning:
        case ElemKind::DynamicLengthBlobBeginning:
            this->_tryCachePaddingRegionBeforeCurIt(curScope);

            // _cacheContentRegionAtCurIt() increments the iterator
            this->_cacheContentRegionAtCurIt(curScope);
            break;

        case ElemKind::ScopeBeginning:
        {
            // cache padding before scope
            this->_tryCachePaddingRegionBeforeCurIt(curScope);

            curScope = std::make_shared<Scope>(curEr, _it->asScopeBeginningElement().scope());
            curScope->segment().offsetInPktBits(this->_itOffsetInPktBits());
            ++_it;
            break;
        }

        case ElemKind::StructureBeginning:
        {
            if (curScope && !curScope->dt()) {
                curScope->dt(_it->asStructureBeginningElement().type());
            }

            ++_it;
            break;
        }

        case ElemKind::ScopeEnd:
            if (curScope) {
                curScope->segment().len(this->_itOffsetInPktBits() -
                                        curScope->segment().offsetInPktBits());
                curScope = nullptr;
            }

            ++_it;
            break;

        case ElemKind::EventRecordBeginning:
            // cache padding before event record
            this->_tryCachePaddingRegionBeforeCurIt(curScope);
            curEr = std::make_shared<Er>(erIndexInPkt);
            curEr->segment().offsetInPktBits(this->_itOffsetInPktBits());

            // immediately cache it because this loop could throw before the end
            _curErCache.push_back(curEr);
            ++_it;
            break;

        case ElemKind::EventRecordEnd:
            if (curEr) {
                curEr->segment().len(this->_itOffsetInPktBits() -
                                     curEr->segment().offsetInPktBits());
                curScope = nullptr;
                curEr = nullptr;
                ++erIndexInPkt;
            }

            ++_it;
            break;

        case ElemKind::EventRecordInfo:
        {
            auto& elem = _it->asEventRecordInfoElement();

            if (curEr && elem.type()) {
                curEr->type(*elem.type());
            }

            ++_it;
            break;
        }

        case ElemKind::DefaultClockValue:
            if (curEr && _metadata->isCorrelatable()) {
                assert(_indexEntry->dst());
                assert(_indexEntry->dst()->defaultClockType());
                curEr->ts(Ts {
                    _it->asDefaultClockValueElement().cycles(),
                    *_indexEntry->dst()->defaultClockType()
                });
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

    assert(_it->isEventRecordBeginningElement());
    this->_cacheRegionsAtCurIt(ElemKind::EventRecordEnd, indexInPkt);
}

void Pkt::_cacheRegionsAtCurItUntilError(const Index initErIndexInPkt)
{
    try {
        this->_cacheRegionsAtCurIt(yactfr::Element::Kind::PacketEnd, initErIndexInPkt);
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

            this->_trySetPrevRegionOffsetInPktBits(*region);
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
    assert(_it->isEventRecordBeginningElement());
    _curRegionCache.clear();
    _curErCache.clear();

    const auto endErIndexInPkt = erIndexInPkt + erCount;
    auto endErIndexInPktBeforeLast = endErIndexInPkt;

    if (_checkpoints.error() && endErIndexInPktBeforeLast == _checkpoints.erCount()) {
        --endErIndexInPktBeforeLast;
    }

    assert(erIndexInPkt <= endErIndexInPktBeforeLast);

    for (auto index = erIndexInPkt; index < endErIndexInPktBeforeLast; ++index) {
        while (!_it->isEventRecordBeginningElement()) {
            assert(!_it->isPacketEndElement());
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
            while (!_it->isPacketEndElement()) {
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

    if (region.prevRegionOffsetInPktBits()) {
        return &this->regionAtOffsetInPktBits(*region.prevRegionOffsetInPktBits());
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
