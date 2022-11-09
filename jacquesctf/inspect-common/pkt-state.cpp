/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "pkt-state.hpp"
#include "data/pkt-region.hpp"
#include "app-state.hpp"

namespace jacques {

PktState::PktState(AppState& appState, const Metadata& metadata, Pkt& pkt) noexcept :
    _appState {&appState},
    _metadata {&metadata},
    _pkt {&pkt}
{
}

void PktState::gotoPrevEr(Size count)
{
    if (_pkt->erCount() == 0) {
        return;
    }

    const auto curEr = this->curEr();

    if (!curEr) {
        if (_curOffsetInPktBits >= _pkt->indexEntry().effectiveContentLen()) {
            auto lastEr = _pkt->lastEr();

            assert(lastEr);
            this->gotoPktRegionAtOffsetInPktBits(lastEr->segment().offsetInPktBits());
        }

        return;
    }

    if (curEr->indexInPkt() == 0) {
        return;
    }

    count = std::min(curEr->indexInPkt(), count);

    const auto& prevEr = _pkt->erAtIndexInPkt(curEr->indexInPkt() - count);

    this->gotoPktRegionAtOffsetInPktBits(prevEr.segment().offsetInPktBits());
}

void PktState::gotoNextEr(Size count)
{
    if (_pkt->erCount() == 0) {
        return;
    }

    const auto curEr = this->curEr();
    Index newIndex = 0;

    if (curEr) {
        count = std::min(_pkt->erCount() - curEr->indexInPkt(), count);
        newIndex = curEr->indexInPkt() + count;
    }

    if (newIndex >= _pkt->erCount()) {
        return;
    }

    const auto& nextEr = _pkt->erAtIndexInPkt(newIndex);

    this->gotoPktRegionAtOffsetInPktBits(nextEr.segment().offsetInPktBits());
}

void PktState::gotoPrevPktRegion()
{
    if (!_pkt->hasData()) {
        return;
    }

    if (_curOffsetInPktBits == 0) {
        return;
    }

    const auto& curPktRegion = this->curPktRegion();

    if (curPktRegion.prevRegionOffsetInPktBits()) {
        this->gotoPktRegionAtOffsetInPktBits(*curPktRegion.prevRegionOffsetInPktBits());
        return;
    }

    const auto& prevPktRegion = _pkt->regionAtOffsetInPktBits(_curOffsetInPktBits - 1);

    this->gotoPktRegionAtOffsetInPktBits(prevPktRegion);
}

void PktState::gotoNextPktRegion()
{
    if (!_pkt->hasData()) {
        return;
    }

    const auto& curPktRegion = this->curPktRegion();

    if (*curPktRegion.segment().endOffsetInPktBits() == _pkt->indexEntry().effectiveTotalLen()) {
        return;
    }

    this->gotoPktRegionAtOffsetInPktBits(*curPktRegion.segment().endOffsetInPktBits());
}

void PktState::gotoPktCtx()
{
    const auto& offset = _pkt->indexEntry().pktCtxOffsetInPktBits();

    if (!offset) {
        return;
    }

    this->gotoPktRegionAtOffsetInPktBits(*offset);
}

void PktState::gotoLastPktRegion()
{
    this->gotoPktRegionAtOffsetInPktBits(_pkt->lastRegion());
}

void PktState::gotoPktRegionAtOffsetInPktBits(const Index offsetInPktBits)
{
    if (offsetInPktBits == _curOffsetInPktBits) {
        return;
    }

    if (offsetInPktBits >= _pkt->indexEntry().effectiveTotalLen()) {
        /*
         * This is a general protection against going too far. This can
         * happen, for example, if an event record begins exactly where
         * the packet content ends, which can happen if yactfr emitted
         * an "event record beginning" element immediately after
         * aligning its cursor.
         *
         * For example, given a packet content of 256 bits, if the
         * current cursor is at 224 bits, and the next event record
         * needs to be aligned to 64 bits, then yactfr aligns its cursor
         * to 256 bits, emits a "event record beginning" element at 256
         * bits, and then eventually returns a decoding error because it
         * cannot decode more. In this case, Jacques CTF considers
         * there's an event record at offset 256, but there's no packet
         * region at offset 256.
         */
        return;
    }

    assert(offsetInPktBits < _pkt->indexEntry().effectiveTotalLen());
    _curOffsetInPktBits = offsetInPktBits;
    _appState->_curOffsetInPktChanged();
}

void PktState::gotoPktRegionNextParent()
{
    if (!_pkt->hasData()) {
        return;
    }

    auto pktRegion = &this->curPktRegion();
    const yactfr::DataType *origParentDt = nullptr;

    if (const auto cRegion = dynamic_cast<const ContentPktRegion *>(pktRegion)) {
        origParentDt = _metadata->dtParent(cRegion->dt());
    } else {
        return;
    }

    assert(origParentDt);

    while (*pktRegion->segment().endOffsetInPktBits() <
            _pkt->indexEntry().effectiveContentLen().bits()) {
        if (const auto cRegion = dynamic_cast<const ContentPktRegion *>(pktRegion)) {
            const auto thisParentDt = _metadata->dtParent(cRegion->dt());

            assert(thisParentDt);

            if (thisParentDt != origParentDt) {
                break;
            }
        }

        pktRegion = &_pkt->regionAtOffsetInPktBits(*pktRegion->segment().endOffsetInPktBits());
    }

    assert(pktRegion);
    this->gotoPktRegionAtOffsetInPktBits(pktRegion->segment().offsetInPktBits());
}

} // namespace jacques
