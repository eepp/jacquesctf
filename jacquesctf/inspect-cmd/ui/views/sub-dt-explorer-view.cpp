/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <sstream>
#include <yactfr/yactfr.hpp>
#include <boost/variant.hpp>

#include "sub-dt-explorer-view.hpp"
#include "data/content-pkt-region.hpp"

namespace jacques {

SubDtExplorerView::SubDtExplorerView(const Rect& rect, const Stylist& stylist,
                                     InspectCmdState& appState) :
    DtExplorerView {rect, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
}

void SubDtExplorerView::_appStateChanged(Message)
{
    const auto& pktRegion = _appState->curPktRegion();

    if (!pktRegion || !pktRegion->scope() || !pktRegion->scope()->dt()) {
        this->reset();
        return;
    }

    if (pktRegion->scope()->er() && pktRegion->scope()->er()->type()) {
        this->ert(*pktRegion->scope()->er()->type());
    } else if (pktRegion->scope()->scope() == yactfr::Scope::PACKET_HEADER ||
            pktRegion->scope()->scope() == yactfr::Scope::PACKET_CONTEXT) {
        const auto dst = _appState->activePktState().pkt().indexEntry().dst();

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
