/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_HPP
#define _JACQUES_DATA_PKT_HPP

#include <algorithm>
#include <memory>
#include <vector>
#include <yactfr/yactfr.hpp>
#include <boost/core/noncopyable.hpp>

#include "pkt-index-entry.hpp"
#include "pkt-checkpoints.hpp"
#include "er.hpp"
#include "pkt-region.hpp"
#include "pkt-segment.hpp"
#include "bit-array.hpp"
#include "content-pkt-region.hpp"
#include "pkt-checkpoints-build-listener.hpp"
#include "metadata.hpp"
#include "mem-mapped-file.hpp"
#include "lru-cache.hpp"

namespace jacques {

/*
 * The purpose of a packet object is to provide packet regions and event
 * records to the user. This is the core data required by any packet
 * inspection activity.
 *
 * We keep a cache of packet regions (content packet regions being
 * linked to event records), and a cache of the event records linked in
 * the data region cache (for a faster access by index).
 *
 * Caching is valuable here because of how a typical packet inspection
 * session induces a locality of reference: you're typically going
 * backward or forward from the offset you're inspecting once you find a
 * location of interest, so there will typically be a lot of cache hits.
 *
 * The packet region cache is a sorted vector of contiguous shared
 * packet regions. The caching operation performed by
 * _ensureErIsCached() makes sure that all the packet regions of at most
 * `_erCacheMaxSize` event records starting at the requested index minus
 * `_erCacheMaxSize / 2` are in cache. Subtracting `_erCacheMaxSize / 2`
 * makes packet regions and event records available "around" the
 * requested index, which makes sense for a packet inspection activity
 * because the user is typically inspecting around a given offset.
 *
 * When a packet object is constructed, it caches everything known to be
 * in the preamble segment, that is, everything before the first event
 * record (if any), or all the regions of the packet otherwise
 * (including any padding or error packet region before the end of the
 * packet). This preamble packet region cache is kept as a separate
 * cache and copied back to the working cache when we request an offset
 * located within it. When the working cache is the preamble cache, the
 * event record cache is empty.
 *
 *     preamble             ER 0          ER 1            ER 2
 *     #################----**********----***********-----********----
 *     ^^^^^^^^^^^^^^^^^^^^^
 *     preamble packet region cache
 *
 *     preamble
 *     #########################------------
 *     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *     preamble packet region cache
 *
 *     preamble                 error packet region
 *     #########################!!!!!!!!!!!!!!!!!!!!!
 *     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *     preamble packet region cache
 *
 *     preamble                           error packet region
 *     #########################----------!!!!!!!!!!!
 *     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *     preamble packet region cache
 *
 * If _cacheRegionsFromErsAtCurIt() adds anything to the packet region
 * cache, and if the last event record to be part of the cache is the
 * last event record of the packet, it also adds any padding or error
 * packet region before the end of the packet:
 *
 *        ER 176   ER 177       ER 294      ER 295         ER 296
 *     ...*******--******----...********----**********-----*******---...
 *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 *        ER 300   ER 301       ER 487        ER 488
 *     ...*******--******----...**********----***********------------
 *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 *        ER 300   ER 301       ER 487
 *     ...*******--******----...**********----********!!!!!!!!!!!!!!!
 *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * Because elements are naturally sorted in the caches, we can perform a
 * binary search to find a specific element using one of its
 * intrinsically ordered properties (index, offset in packet,
 * timestamp).
 *
 * As of this version, when setting the preamble region cache as the
 * current cache, we move the current region cache (and current event
 * record cache) to temporary members from which the current caches can
 * be restored. This can be viewed as a double buffer, where you can
 * simultaneously have the preamble region cache and the last region
 * cache available. This helps in scenarios where the non-preamble
 * region cache is huge (contains a single, incomplete event with a
 * somewhat huge total packet length, for example) to avoid creating
 * this huge cache everytime the requests alternate between the preamble
 * region cache and the non-preamble region cache. This could be
 * generalized with an LRU cache of packet region caches.
 *
 * There's also an LRU cache (offset in packet to packet region) for
 * frequently accessed packet regions by offset (with
 * regionAtOffsetInPktBits()): when there's a cache miss, the method
 * calls _ensureOffsetInPktBitsIsCached() to update the packet region
 * and event record caches and then adds the packet region entry to the
 * LRU cache. The LRU cache avoids performing a binary search by
 * _regionCacheItBeforeOrAtOffsetInPktBits() every time.
 */
class Pkt final :
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<Pkt>;

public:
    explicit Pkt(const PktIndexEntry& indexEntry, yactfr::ElementSequence& seq,
                 const Metadata& metadata, yactfr::DataSource::UP dataSrc,
                 std::unique_ptr<MemMappedFile> mmapFile,
                 PktCheckpointsBuildListener& pktCheckpointsBuildListener);

    /*
     * Appends packet regions to `regions` (calling
     * ContainerT::push_back()) from `offsetInPktBits` to
     * `endOffsetInPktBits` (excluded).
     */
    template <typename ContainerT>
    void appendRegions(ContainerT& regions, const Index offsetInPktBits,
                       const Index endOffsetInPktBits)
    {
        assert(offsetInPktBits < _indexEntry->effectiveTotalLen());
        assert(endOffsetInPktBits <= _indexEntry->effectiveTotalLen());
        assert(offsetInPktBits < endOffsetInPktBits);

        auto curOffsetInPktBits = offsetInPktBits;

        while (true) {
            this->_ensureOffsetInPktBitsIsCached(curOffsetInPktBits);

            auto it = this->_regionCacheItBeforeOrAtOffsetInPktBits(curOffsetInPktBits);

            /*
             * If `curOffsetInPktBits` was the exact offset of a packet
             * region, then it's unchanged here.
             */
            curOffsetInPktBits = (*it)->segment().offsetInPktBits();

            while (true) {
                if (it == _curRegionCache.end()) {
                    // need to cache more
                    break;
                }

                this->_appendConstRegion(regions, it);
                curOffsetInPktBits = *(*it)->segment().endOffsetInPktBits();
                ++it;

                if (curOffsetInPktBits >= endOffsetInPktBits) {
                    return;
                }
            }
        }
    }

    /*
     * Returned region is guaranteed to be valid until you call another
     * `Pkt` method on the same packet object.
     */
    const PktRegion& regionAtOffsetInPktBits(Index offsetInPktBits);

    /*
     * Returned region is guaranteed to be valid until you call another
     * `Pkt` method on the same packet object.
     */
    const PktRegion *previousRegion(const PktRegion& region);

    /*
     * Returned region is guaranteed to be valid until you call another
     * `Pkt` method on this packet object.
     */
    const PktRegion& firstRegion();

    /*
     * Returned region is guaranteed to be valid until you call another
     * `Pkt` method on this packet object.
     */
    const PktRegion& lastRegion();

    /*
     * Returned event record is guaranteed to be valid until you call
     * another `Pkt` method on this packet object.
     */
    const Er *erBeforeOrAtNsFromOrigin(long long nsFromOrigin);

    /*
     * Returned event record is guaranteed to be valid until you call
     * another `Pkt` method on this packet object.
     */
    const Er *erBeforeOrAtCycles(unsigned long long cycles);

    /*
     * Returned bit array remains valid as long as this packet object
     * exists.
     */
    BitArray bitArray(const PktSegment& segment) const noexcept
    {
        assert(segment.len());

        return BitArray {
            _mmapFile->addr() + segment.offsetInPktBits() / 8,
            segment.offsetInFirstByteBits(),
            *segment.len(),
            segment.bo()
        };
    }

    /*
     * Returned bit array remains valid as long as this packet object
     * exists.
     */
    BitArray bitArray(const PktRegion& region) const noexcept
    {
        return this->bitArray(region.segment());
    }

    /*
     * Returned bit array remains valid as long as this packet object
     * exists.
     */
    BitArray bitArray(const Scope& scope) const noexcept
    {
        return this->bitArray(scope.segment());
    }

    /*
     * Returned bit array remains valid as long as this packet object
     * exists.
     */
    BitArray bitArray(const Er& er) const noexcept
    {
        return this->bitArray(er.segment());
    }

    /*
     * Returned data remains valid as long as this packet object exists.
     */
    const std::uint8_t *data(const Index offsetInPktBytes) const
    {
        assert(offsetInPktBytes < _indexEntry->effectiveTotalLen().bytes());
        return _mmapFile->addr() + offsetInPktBytes;
    }

    bool hasData() const noexcept
    {
        return _indexEntry->effectiveTotalLen() > 0;
    }

    /*
     * Returned index entry remains valid as long as this packet object
     * exists.
     */
    const PktIndexEntry& indexEntry() const noexcept
    {
        return *_indexEntry;
    }

    Size erCount() const noexcept
    {
        return _checkpoints.erCount();
    }

    const boost::optional<PktDecodingError>& error() const noexcept
    {
        return _checkpoints.error();
    }

    const Er& erAtIndexInPkt(const Index reqIndexInPkt)
    {
        assert(reqIndexInPkt < _checkpoints.erCount());
        this->_ensureErIsCached(reqIndexInPkt);
        return **this->_erCacheItFromIndexInPkt(reqIndexInPkt);
    }

    /*
     * Returned event record is guaranteed to be valid until you call
     * another `Pkt` method on this packet object.
     */
    const Er *firstEr() const
    {
        if (_checkpoints.erCount() == 0) {
            return nullptr;
        }

        return _checkpoints.firstEr().get();
    }

    /*
     * Returned event record is guaranteed to be valid until you call
     * another `Pkt` method on this packet object.
     */
    const Er *lastEr() const
    {
        if (_checkpoints.erCount() == 0) {
            return nullptr;
        }

        return _checkpoints.lastEr().get();
    }

private:
    using _RegionCache = std::vector<PktRegion::SP>;
    using _ErCache = std::vector<Er::SP>;

private:
    /*
     * Caches the whole packet preamble (single time): packet header,
     * packet context, and any padding until the first event record (if
     * any) or any padding/error until the end of the packet.
     *
     * Clears the event record cache.
     */
    void _cachePreambleRegions();

    /*
     * Makes sure that the event record at index `indexInPkt` exists in
     * the caches. If it doesn't exist, then this method caches the
     * requested event record as well as half of `_erCacheMaxSize` event
     * records before and after (if possible), "centering" the requested
     * event record within its cache.
     */
    void _ensureErIsCached(Index indexInPkt);

    /*
     * Makes sure that a packet region containing the bit
     * `offsetInPktBits` exists in cache. If it doesn't exist, the
     * method finds the closest event record containing this bit and
     * calls _ensureErIsCached() with its index.
     */
    void _ensureOffsetInPktBitsIsCached(Index offsetInPktBits);

    /*
     * Appends all the remaining packet regions starting at the current
     * iterator until any decoding error, and then an error packet
     * region.
     */
    void _cacheRegionsAtCurItUntilError(Index initErIndexInPkt);

    /*
     * After clearing the caches, caches all the packet regions from the
     * event records starting at the current iterator, for `erCount`
     * event records.
     */
    void _cacheRegionsFromErsAtCurIt(Index erIndexInPkt, Size erCount);

    /*
     * Appends a single event record (having index `indexInPkt`) worth
     * of packet regions to the cache starting at the current iterator.
     */
    void _cacheRegionsFromOneErAtCurIt(Index indexInPkt);

    /*
     * Appends packet regions to the packet region cache (and updates
     * the event record cache if needed) starting at the current
     * iterator. Stops appending _after_ the kind of the current element
     * of the iterator is `endElemKind`.
     */
    void _cacheRegionsAtCurIt(yactfr::Element::Kind endElemKind, Index erIndexInPkt);

    /*
     * Tries to append a padding packet region to the current cache,
     * where this packet region would be located just before the current
     * iterator. This method uses the last packet region of the current
     * cache to know if there's padding, and if there is, what should be
     * its byte order. The padding packet region is assigned scope
     * `scope` (may be `nullptr`).
     */
    void _tryCachePaddingRegionBeforeCurIt(Scope::SP scope);

    /*
     * Appends a content packet region to the current cache from the
     * element(s) at the current iterator. Increments the current
     * iterator so that it contains the following element. The content
     * packet region is assigned scope `scope` (may not be `nullptr`).
     */
    void _cacheContentRegionAtCurIt(Scope::SP scope);

    /*
     * Returns whether or not the packet region cache `cache` contains
     * the bit `offsetInPktBits`.
     */
    bool _regionCacheContainsOffsetInPktBits(const _RegionCache& cache,
                                             const Index offsetInPktBits) const
    {
        if (cache.empty()) {
            return false;
        }

        return offsetInPktBits >= cache.front()->segment().offsetInPktBits() &&
               offsetInPktBits < *cache.back()->segment().endOffsetInPktBits();
    }

    /*
     * Returns the packet region, within the current cache, of which the
     * offset is less than or equal to `offsetInPktBits`.
     */
    _RegionCache::const_iterator _regionCacheItBeforeOrAtOffsetInPktBits(const Index offsetInPktBits)
    {
        assert(!_curRegionCache.empty());
        assert(this->_regionCacheContainsOffsetInPktBits(_curRegionCache, offsetInPktBits));

        const auto lessThanFunc = [](const auto& offsetInPktBits, const auto region) {
            return offsetInPktBits < region->segment().offsetInPktBits();
        };

        auto it = std::upper_bound(_curRegionCache.begin(), _curRegionCache.end(), offsetInPktBits,
                                   lessThanFunc);

        assert(it != _curRegionCache.begin());

        // we found one that is greater than, decrement once to find <=
        --it;
        assert((*it)->segment().offsetInPktBits() <= offsetInPktBits);
        return it;
    }

    /*
     * Returns the event record cache iterator containing the event
     * record having the index `indexInPkt`.
     */
    _ErCache::const_iterator _erCacheItFromIndexInPkt(const Index indexInPkt)
    {
        assert(!_curErCache.empty());

        auto it = _curErCache.begin();

        assert(indexInPkt >= (*it)->indexInPkt() &&
               indexInPkt < (*it)->indexInPkt() + _curErCache.size());
        it += (indexInPkt - (*it)->indexInPkt());
        return it;
    }

    /*
     * Returns whether or not the event record having the index
     * `indexInPkt` exists in the caches.
     */
    bool _erIsCached(const _ErCache& erCache, const Index indexInPkt) const
    {
        if (erCache.empty()) {
            return false;
        }

        auto& firstEr = *erCache.front();
        auto& lastEr = *erCache.back();

        return indexInPkt >= firstEr.indexInPkt() && indexInPkt <= lastEr.indexInPkt();
    }

    /*
     * Offset (bits) of current iterator within the packet.
     */
    Index _itOffsetInPktBits() const noexcept
    {
        return _it.offset() - _indexEntry->offsetInDsFileBits();
    }

    /*
     * Offset (bytes, floored) of current iterator within the packet.
     */
    Index _itOffsetInPktBytes() const noexcept
    {
        return this->_itOffsetInPktBits() / 8;
    }

    static Size _bitArrayElemLen(const yactfr::FixedLengthBitArrayElement& elem) noexcept
    {
        return elem.type().length();
    }

    static Size _bitArrayElemLen(const yactfr::VariableLengthBitArrayElement& elem) noexcept
    {
        // `elem.length()` is the length of the decoded bits
        return elem.length() + elem.length() / 7;
    }

    /*
     * Creates and returns a content packet region from the bit array
     * element of the current iterator known to have the type `ElemT`
     * and having the value `val`.
     *
     * The created content packet region is assigned scope `scope` (may
     * not be `nullptr`).
     */
    template <typename ElemT, typename ValT>
    ContentPktRegion::SP _contentRegionFromBitArrayElemAtCurIt(Scope::SP scope, const ValT val)
    {
        assert(scope);

        auto& elem = static_cast<const ElemT&>(*_it);
        const PktSegment segment {this->_itOffsetInPktBits(), Pkt::_bitArrayElemLen(elem)};

        return std::make_shared<ContentPktRegion>(segment, std::move(scope), elem.type(),
                                                  ContentPktRegion::Val {val});
    }

    /*
     * Creates and returns a content packet region from the bit array
     * element of the current iterator known to have the type `ElemT`.
     *
     * The content packet region is assigned scope `scope` (may not be
     * `nullptr`).
     */
    template <typename ElemT>
    ContentPktRegion::SP _contentRegionFromBitArrayElemAtCurIt(Scope::SP scope)
    {
        auto& elem = static_cast<const ElemT&>(*_it);

        return this->_contentRegionFromBitArrayElemAtCurIt<ElemT>(std::move(scope), elem.value());
    }

    void _trySetPrevRegionOffsetInPktBits(PktRegion& region) const
    {
        if (_curRegionCache.empty()) {
            return;
        }

        region.prevRegionOffsetInPktBits(_curRegionCache.back()->segment().offsetInPktBits());
    }

    template <typename ContainerT, typename IterT>
    void _appendConstRegion(ContainerT& regions, const IterT& it) const
    {
        regions.push_back(std::static_pointer_cast<const PktRegion>(*it));
    }

    template <typename CpNearestFuncT, typename GetProcFuncT, typename PropT>
    const Er *_erBeforeOrAtTs(CpNearestFuncT&& cpNearestFunc, GetProcFuncT&& getProcFuncT,
                              const PropT prop)
    {
        if (!_metadata->isCorrelatable()) {
            return nullptr;
        }

        if (_checkpoints.erCount() == 0) {
            return nullptr;
        }

        const auto er = _checkpoints.lastEr();

        assert(er);

        if (er->ts() && _indexEntry->endTs() &&
                prop >= getProcFuncT(*er->ts()) &&
                prop < getProcFuncT(*_indexEntry->endTs())) {
            // special case: between last event record and end of packet
            return er.get();
        }

        const auto cp = cpNearestFunc(prop);

        if (!cp) {
            return nullptr;
        }

        _it.restorePosition(cp->second);

        auto curIndex = cp->first->indexInPkt();
        auto inEr = false;
        boost::optional<Ts> ts;
        boost::optional<Index> indexInPkt;

        while (_it != _endIt) {
            if (_it->isEventRecordBeginningElement()) {
                inEr = true;
                ts = boost::none;
                ++_it;
            } else if (_it->isDefaultClockValueElement()) {
                if (inEr) {
                    auto& elem = _it->asDefaultClockValueElement();

                    assert(_indexEntry->dst());
                    assert(_indexEntry->dst()->defaultClockType());
                    ts = Ts {elem.cycles(), *_indexEntry->dst()->defaultClockType()};
                }

                ++_it;
            } else if (_it->isEventRecordInfoElement()) {
                const auto erProp = getProcFuncT(*ts);

                if (erProp == prop) {
                    indexInPkt = curIndex;
                    _it = _endIt;
                } else if (erProp > prop) {
                    // we're looking for the previous one
                    if (curIndex == 0) {
                        return nullptr;
                    }

                    indexInPkt = curIndex - 1;
                    _it = _endIt;
                } else {
                    ++_it;
                }
            } else if (_it->isEventRecordEndElement()) {
                inEr = false;
                ++curIndex;
                ++_it;
            } else {
                ++_it;
            }
        }

        if (!indexInPkt || *indexInPkt > _checkpoints.erCount()) {
            return nullptr;
        }

        this->_ensureErIsCached(*indexInPkt);
        return _erCacheItFromIndexInPkt(*indexInPkt)->get();
    }

private:
    const PktIndexEntry * const _indexEntry;
    const Metadata * const _metadata;
    yactfr::DataSource::UP _dataSrc;
    std::unique_ptr<MemMappedFile> _mmapFile;
    yactfr::ElementSequenceIterator _it;
    yactfr::ElementSequenceIterator _endIt;
    PktCheckpoints _checkpoints;
    _RegionCache _preambleRegionCache;
    _RegionCache _curRegionCache;
    _ErCache _curErCache;
    _RegionCache _lastRegionCache;
    _ErCache _lastErCache;
    LruCache<Index, PktRegion::SP> _lruRegionCache;
    const Size _erCacheMaxSize = 500;
    const DataLen _preambleLen;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_HPP
