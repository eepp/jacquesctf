/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <sstream>
#include <yactfr/metadata/event-record-type.hpp>
#include <yactfr/metadata/data-stream-type.hpp>
#include <boost/variant.hpp>

#include "sub-dt-explorer-view.hpp"
#include "data/content-pkt-region.hpp"

namespace jacques {

SubDtExplorerView::SubDtExplorerView(const Rect& rect, const Stylist& stylist, State& state) :
    DtExplorerView {rect, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
}

void SubDtExplorerView::_stateChanged(Message)
{
    const auto& pktRegion = _state->curPktRegion();

    if (!pktRegion || !pktRegion->scope() || !pktRegion->scope()->dt()) {
        this->reset();
        return;
    }

    if (pktRegion->scope()->er() && pktRegion->scope()->er()->type()) {
        this->ert(*pktRegion->scope()->er()->type());
    } else if (pktRegion->scope()->scope() == yactfr::Scope::PACKET_HEADER ||
            pktRegion->scope()->scope() == yactfr::Scope::PACKET_CONTEXT) {
        const auto dst = _state->activePktState().pkt().indexEntry().dst();

        if (dst) {
            this->dst(*dst, false);
        } else {
            this->singleDt(*pktRegion->scope()->dt(), pktRegion->scope()->scope());
        }
    } else {
        this->singleDt(*pktRegion->scope()->dt(), pktRegion->scope()->scope());
    }

    if (const auto region = dynamic_cast<const ContentPktRegion *>(pktRegion)) {
        this->highlightDt(region->dt());
        this->centerHighlight();
    } else {
        this->clearHighlight();
    }

    this->redraw();
}

} // namespace jacques
