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

#include "sub-data-type-explorer-view.hpp"
#include "content-packet-region.hpp"

namespace jacques {

SubDataTypeExplorerView::SubDataTypeExplorerView(const Rectangle& rect,
                                                 const Stylist& stylist,
                                                 State& state) :
    DataTypeExplorerView {rect, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
}

void SubDataTypeExplorerView::_stateChanged(const Message)
{
    const auto& packetRegion = _state->currentPacketRegion();

    if (!packetRegion || !packetRegion->scope() ||
            !packetRegion->scope()->dataType()) {
        this->reset();
        return;
    }

    if (packetRegion->scope()->eventRecord() &&
            packetRegion->scope()->eventRecord()->type()) {
        this->eventRecordType(*packetRegion->scope()->eventRecord()->type());
    } else if (packetRegion->scope()->scope() == yactfr::Scope::PACKET_HEADER ||
            packetRegion->scope()->scope() == yactfr::Scope::PACKET_CONTEXT) {
        const auto dst = _state->activePacketState().packet().indexEntry().dataStreamType();

        if (dst) {
            this->dataStreamType(*dst, false);
        } else {
            this->singleDataType(*packetRegion->scope()->dataType(),
                                 packetRegion->scope()->scope());
        }
    } else {
        this->singleDataType(*packetRegion->scope()->dataType(),
                             packetRegion->scope()->scope());
    }

    if (const auto region = dynamic_cast<const ContentPacketRegion *>(packetRegion)) {
        this->highlightDataType(region->dataType());
        this->centerHighlight();
    } else {
        this->clearHighlight();
    }

    this->redraw();
}

} // namespace jacques
