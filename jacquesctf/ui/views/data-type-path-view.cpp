/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cinttypes>
#include <cstdio>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

#include "stylist.hpp"
#include "data-type-path-view.hpp"
#include "utils.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
#include "content-data-region.hpp"

namespace jacques {

DataTypePathView::DataTypePathView(const Rectangle& rect,
                                   std::shared_ptr<const Stylist> stylist,
                                   std::shared_ptr<State> state) :
    View {rect, "Data type path", DecorationStyle::BORDERLESS, stylist},
    _state {state},
    _stateObserverGuard {*state, *this}
{
}

void DataTypePathView::_stateChanged(const Message& msg)
{
    this->redraw();
}

void DataTypePathView::_redrawContent()
{
    // clear
    this->_stylist().dataTypePathViewStd(*this);
    this->_clearRect();

    const auto dataRegion = _state->currentDataRegion();

    if (!dataRegion) {
        return;
    }

    const auto cDataRegion = dynamic_cast<const ContentDataRegion *>(dataRegion);

    if (!cDataRegion) {
        return;
    }

    const auto& path = _state->metadata().dataTypePath(cDataRegion->dataType());

    this->_moveCursor({0, 0});

    if (path.path.empty()) {
        this->_stylist().dataTypePathViewStd(*this, true);
    }

    switch (path.scope) {
    case yactfr::Scope::PACKET_HEADER:
        this->_print("PH");
        break;

    case yactfr::Scope::PACKET_CONTEXT:
        this->_print("PC");
        break;

    case yactfr::Scope::EVENT_RECORD_HEADER:
        this->_print("ERH");
        break;

    case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
        this->_print("ER1C");
        break;

    case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
        this->_print("ER2C");
        break;

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        this->_print("ERP");
        break;
    }

    if (path.path.empty()) {
        this->_stylist().dataTypePathViewStd(*this);
    }

    for (auto it = std::begin(path.path); it != std::end(path.path); ++it) {
        this->_print("/");

        if (it == std::end(path.path) - 1) {
            this->_stylist().dataTypePathViewStd(*this, true);
        }

        this->_print("%s", it->c_str());
    }
}

} // namespace jacques
