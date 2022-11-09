/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ds-file-state.hpp"
#include "search-query.hpp"
#include "app-state.hpp"
#include "io-error.hpp"

namespace jacques {

DsFileState::DsFileState(AppState& appState, DsFile& dsFile,
                         PktCheckpointsBuildListener& pktCheckpointsBuildListener) :
    _appState {&appState},
    _pktCheckpointsBuildListener {&pktCheckpointsBuildListener},
    _dsFile {&dsFile}
{
}

void DsFileState::gotoOffsetBits(const Index offsetBits)
{
    if (!_dsFile->hasOffsetBits(offsetBits)) {
        return;
    }

    const auto& pktIndexEntry = _dsFile->pktIndexEntryContainingOffsetBits(offsetBits);

    this->gotoPkt(pktIndexEntry.indexInDsFile());

    const auto offsetInPktBits = offsetBits - pktIndexEntry.offsetInDsFileBits();

    if (offsetInPktBits > pktIndexEntry.effectiveTotalLen()) {
        // uh oh, that's outside the data we have for this invalid packet
        this->gotoLastPktRegion();
        return;
    }

    const auto& region = _activePktState->pkt().regionAtOffsetInPktBits(offsetInPktBits);

    _activePktState->gotoPktRegionAtOffsetInPktBits(region);
}

PktState& DsFileState::_pktState(const Index index)
{
    if (_pktStates.size() < index + 1) {
        _pktStates.resize(index + 1);
    }

    if (!_pktStates[index]) {
        auto& pkt = _dsFile->pktAtIndex(index, *_pktCheckpointsBuildListener);

        _pktStates[index] = std::make_unique<PktState>(*_appState, _dsFile->metadata(), pkt);
    }

    return *_pktStates[index];
}

void DsFileState::_gotoPkt(const Index index)
{
    assert(index < _dsFile->pktCount());
    _activePktStateIndex = index;
    _activePktState = &this->_pktState(index);
    _appState->_activePktChanged();
}

void DsFileState::gotoPkt(const Index index)
{
    assert(index < _dsFile->pktCount());

    if (!_activePktState) {
        // special case for the very first one, no notification required
        assert(index == 0);
        this->_gotoPkt(index);
        return;
    }

    if (_activePktStateIndex == index) {
        return;
    }

    this->_gotoPkt(index);
}

void DsFileState::gotoPrevPkt()
{
    if (_dsFile->pktCount() == 0) {
        return;
    }

    if (_activePktStateIndex == 0) {
        return;
    }

    this->gotoPkt(_activePktStateIndex - 1);
}

void DsFileState::gotoNextPkt()
{
    if (_dsFile->pktCount() == 0) {
        return;
    }

    if (_activePktStateIndex == _dsFile->pktCount() - 1) {
        return;
    }

    this->gotoPkt(_activePktStateIndex + 1);
}

void DsFileState::gotoPrevEr(Size count)
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoPrevEr(count);
}

void DsFileState::gotoNextEr(Size count)
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoNextEr(count);
}

void DsFileState::gotoPrevPktRegion()
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoPrevPktRegion();
}

void DsFileState::gotoNextPktRegion()
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoNextPktRegion();
}

void DsFileState::gotoPktCtx()
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoPktCtx();
}

void DsFileState::gotoLastPktRegion()
{
    if (!_activePktState) {
        return;
    }

    _activePktState->gotoLastPktRegion();
}

bool DsFileState::_gotoNextErWithProp(const std::function<bool (const Er&)>& cmpFunc,
                                      const boost::optional<Index>& initPktIndex,
                                      const boost::optional<Index>& initErIndex)
{
    if (!_activePktState) {
        return false;
    }

    Index startPktIndex = _activePktStateIndex + 1;
    boost::optional<Index> startErIndex = 0;

    if (initPktIndex) {
        startPktIndex = *initPktIndex;
    }

    if (initErIndex) {
        startErIndex = *initErIndex;
    }

    if (_activePktState->pkt().erCount() > 0 && !initPktIndex && !initErIndex) {
        const auto curEr = _activePktState->curEr();
        boost::optional<Index> erIndex;

        if (curEr) {
            if (curEr->indexInPkt() < _activePktState->pkt().erCount() - 1) {
                // skip current event record
                startPktIndex = _activePktStateIndex;
                startErIndex = curEr->indexInPkt() + 1;
            }
        } else {
            const auto& firstEr = _activePktState->pkt().erAtIndexInPkt(0);

            if (_activePktState->curOffsetInPktBits() <
                    firstEr.segment().offsetInPktBits()) {
                // search active packet from beginning
                startPktIndex = _activePktStateIndex;
            }
        }
    }

    for (auto pktIndex = startPktIndex; pktIndex < _dsFile->pktCount(); ++pktIndex) {
        auto& pkt = this->_pktState(pktIndex).pkt();

        const auto itStartErIndex = startErIndex ? *startErIndex : 0;

        startErIndex = boost::none;
        assert(itStartErIndex < pkt.erCount());

        for (Index erIndex = itStartErIndex; erIndex < pkt.erCount(); ++erIndex) {
            const auto& er = pkt.erAtIndexInPkt(erIndex);

            if (cmpFunc(er)) {
                const auto offsetInPktBits = er.segment().offsetInPktBits();

                this->gotoPkt(pktIndex);
                _activePktState->gotoPktRegionAtOffsetInPktBits(offsetInPktBits);
                return true;
            }
        }
    }

    return false;
}

bool DsFileState::search(const SearchQuery& query)
{
    if (const auto sQuery = dynamic_cast<const PktIndexSearchQuery *>(&query)) {
        long long reqIndex;

        if (sQuery->isDiff()) {
            reqIndex = static_cast<long long>(_activePktStateIndex) + sQuery->val();
        } else {
            // entry is natural (1-based)
            reqIndex = sQuery->val() - 1;
        }

        if (reqIndex < 0) {
            return false;
        }

        const auto index = static_cast<Index>(reqIndex);

        if (index >= _dsFile->pktCount()) {
            return false;
        }

        this->gotoPkt(index);
        return true;
    } else if (const auto sQuery = dynamic_cast<const PktSeqNumSearchQuery *>(&query)) {
        if (!_activePktState) {
            return false;
        }

        long long reqSeqNum;

        if (sQuery->isDiff()) {
            const auto& indexEntry = _activePktState->pktIndexEntry();

            if (!indexEntry.seqNum()) {
                return false;
            }

            reqSeqNum = static_cast<long long>(*indexEntry.seqNum()) + sQuery->val();
        } else {
            reqSeqNum = sQuery->val();
        }

        if (reqSeqNum < 0) {
            return false;
        }

        const auto indexEntry = _dsFile->pktIndexEntryWithSeqNum(static_cast<Index>(reqSeqNum));

        if (!indexEntry) {
            return false;
        }

        this->gotoPkt(indexEntry->indexInDsFile());
        return true;
    } else if (const auto sQuery = dynamic_cast<const ErIndexSearchQuery *>(&query)) {
        if (!_activePktState) {
            return false;
        }

        long long reqIndex;

        if (sQuery->isDiff()) {
            const auto curEr = _activePktState->curEr();

            if (!curEr) {
                return false;
            }

            reqIndex = static_cast<long long>(curEr->indexInPkt()) + sQuery->val();
        } else {
            // entry is natural (1-based)
            reqIndex = sQuery->val() - 1;
        }

        if (reqIndex < 0) {
            return false;
        }

        const auto index = static_cast<Index>(reqIndex);

        if (index >= _activePktState->pkt().erCount()) {
            return false;
        }

        const auto& er = _activePktState->pkt().erAtIndexInPkt(index);

        this->gotoPktRegionAtOffsetInPktBits(er.segment().offsetInPktBits());
        return true;
    } else if (const auto sQuery = dynamic_cast<const OffsetSearchQuery *>(&query)) {
        auto reqOffsetBits = sQuery->val();

        if (sQuery->target() == OffsetSearchQuery::Target::PKT && !_activePktState) {
            return false;
        }

        if (sQuery->isDiff()) {
            switch (sQuery->target()) {
            case OffsetSearchQuery::Target::PKT:
                reqOffsetBits += static_cast<long long>(_activePktState->curOffsetInPktBits());
                break;

            case OffsetSearchQuery::Target::DS_FILE:
            {
                const auto curPktOffsetBitsInDsFile = _activePktState->pktIndexEntry().offsetInDsFileBits();

                reqOffsetBits += static_cast<long long>(curPktOffsetBitsInDsFile +
                                                        _activePktState->curOffsetInPktBits());
                break;
            }
            }
        }

        if (reqOffsetBits < 0) {
            return false;
        }

        const auto offsetInPktBits = static_cast<Index>(reqOffsetBits);

        switch (sQuery->target()) {
        case OffsetSearchQuery::Target::PKT:
        {
            if (offsetInPktBits >= _activePktState->pktIndexEntry().effectiveTotalLen()) {
                return false;
            }

            const auto& region = _activePktState->pkt().regionAtOffsetInPktBits(offsetInPktBits);

            _activePktState->gotoPktRegionAtOffsetInPktBits(region);
            break;
        }

        case OffsetSearchQuery::Target::DS_FILE:
            this->gotoOffsetBits(offsetInPktBits);
            break;
        }

        return true;
    } else if (const auto sQuery = dynamic_cast<const ErtIdSearchQuery *>(&query)) {
        if (sQuery->val() < 0) {
            return false;
        }

        const auto cmpFunc = [sQuery](const Er& er) {
            if (!er.type()) {
                return false;
            }

            return er.type()->id() == static_cast<Index>(sQuery->val());
        };

        return this->_gotoNextErWithProp(cmpFunc);
    } else if (const auto sQuery = dynamic_cast<const ErtNameSearchQuery *>(&query)) {
        const auto cmpFunc = [sQuery](const Er& er) {
            if (!er.type()) {
                return false;
            }

            if (!er.type()->name()) {
                return false;
            }

            return sQuery->matches(*er.type()->name());
        };

        return this->_gotoNextErWithProp(cmpFunc);
    } else if (const auto sQuery = dynamic_cast<const TimestampSearchQuery *>(&query)) {
        if (!_activePktState) {
            return false;
        }

        auto reqVal = sQuery->val();

        if (sQuery->isDiff()) {
            const auto curEr = _activePktState->curEr();

            if (!curEr || !curEr->ts()) {
                return false;
            }

            switch (sQuery->unit()) {
            case TimestampSearchQuery::Unit::NS:
                reqVal += curEr->ts()->nsFromOrigin();
                break;

            case TimestampSearchQuery::Unit::CYCLE:
                reqVal += static_cast<long long>(curEr->ts()->cycles());
                break;
            }
        }

        if (sQuery->unit() == TimestampSearchQuery::Unit::CYCLE && reqVal < 0) {
            return false;
        }

        const PktIndexEntry *indexEntry = nullptr;

        switch (sQuery->unit()) {
        case TimestampSearchQuery::Unit::NS:
            indexEntry = _dsFile->pktIndexEntryContainingNsFromOrigin(reqVal);
            break;

        case TimestampSearchQuery::Unit::CYCLE:
            indexEntry = _dsFile->pktIndexEntryContainingCycles(static_cast<unsigned long long>(reqVal));
            break;
        }

        if (!indexEntry) {
            return false;
        }

        auto& pkt = _dsFile->pktAtIndex(indexEntry->indexInDsFile(), *_pktCheckpointsBuildListener);

        if (pkt.erCount() == 0) {
            return false;
        }

        const Er *er = nullptr;

        switch (sQuery->unit()) {
        case TimestampSearchQuery::Unit::NS:
            er = pkt.erBeforeOrAtNsFromOrigin(reqVal);
            break;

        case TimestampSearchQuery::Unit::CYCLE:
            er = pkt.erBeforeOrAtCycles(static_cast<unsigned long long>(reqVal));
            break;
        }

        if (!er) {
            return false;
        }

        // `er` can become invalid through `this->gotoPkt()`
        const auto offsetInPktBits = er->segment().offsetInPktBits();

        if (_activePktState->pkt().indexEntry() != *indexEntry) {
            // change packet
            this->gotoPkt(indexEntry->indexInDsFile());
        }

        _activePktState->gotoPktRegionAtOffsetInPktBits(offsetInPktBits);
        return true;
    }

    return false;
}

void DsFileState::analyzeAllPkts(PktCheckpointsBuildListener *buildListener)
{
    if (!buildListener) {
        buildListener = _pktCheckpointsBuildListener;
    }

    for (auto& pktIndexEntry : _dsFile->pktIndexEntries()) {
        if (pktIndexEntry.erCount()) {
            continue;
        }

        // this creates checkpoints and shows progress
        _dsFile->pktAtIndex(pktIndexEntry.indexInDsFile(), *buildListener);
    }
}

} // namespace jacques
