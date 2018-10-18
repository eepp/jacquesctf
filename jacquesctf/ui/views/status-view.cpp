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
#include "status-view.hpp"
#include "utils.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
#include "cur-offset-in-packet-changed-message.hpp"

namespace jacques {

StatusView::StatusView(const Rectangle& rect,
                       std::shared_ptr<const Stylist> stylist,
                       std::shared_ptr<State> state) :
    View {rect, "Status", DecorationStyle::BORDERLESS, stylist},
    _state {state},
    _stateObserverGuard {*state, *this}
{
}

void StatusView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg) ||
            dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        this->redraw();
    } else if (dynamic_cast<const CurOffsetInPacketChangedMessage *>(&msg)) {
        this->_drawOffset();
    }
}

void StatusView::_drawOffset()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_stylist().statusViewStd(*this);

    for (auto x = this->contentRect().w - 47; x < this->contentRect().w - 25; ++x) {
        this->_putChar({x, 0}, ' ');
    }

    this->_moveAndPrint({this->contentRect().w - 47, 0}, "{");
    this->_stylist().statusViewStd(*this, true);
    this->_moveAndPrint({this->contentRect().w - 46, 0}, "%s",
                        utils::sepNumber(_state->activePacketState().curOffsetInPacketBits(),
                                         ',').c_str());
    this->_stylist().statusViewStd(*this);
    this->_print(" b}");
}

void StatusView::_redrawContent()
{
    // clear
    this->_stylist().statusViewStd(*this);
    this->_clearRect();

    // packet index
    std::array<char, 32> packetCount;
    std::array<char, 32> curPacket;

    if (_state->hasActivePacketState()) {
        const auto index = _state->activePacketState().packetIndexEntry().natIndexInDataStream();

        std::snprintf(curPacket.data(), curPacket.size(), "%s",
                      utils::sepNumber(static_cast<long long>(index), ',').c_str());
    } else {
        std::strcpy(curPacket.data(), "");
    }

    const auto count = _state->activeDataStreamFileState().dataStreamFile().packetCount();

    std::snprintf(packetCount.data(), packetCount.size(), "/%s",
                  utils::sepNumber(count, ',').c_str());

    auto pktInfoPos = Point {
        this->contentRect().w - std::strlen(packetCount.data()) - std::strlen(curPacket.data()) - 1,
        0
    };

    this->_putChar(pktInfoPos, '#');
    this->_stylist().statusViewStd(*this, true);
    ++pktInfoPos.x;
    this->_moveAndPrint(pktInfoPos, "%s", curPacket.data());
    this->_stylist().statusViewStd(*this);
    pktInfoPos.x += std::strlen(curPacket.data());
    this->_moveAndPrint(pktInfoPos, "%s", packetCount.data());

    // packet sequence number
    if (_state->hasActivePacketState()) {
        const auto& activePacketState = _state->activePacketState();
        const auto& seqNum = activePacketState.packetIndexEntry().seqNum();

        if (seqNum) {
            this->_moveAndPrint({this->contentRect().w - 25, 0}, "##");
            this->_stylist().statusViewStd(*this, true);
            this->_moveAndPrint({this->contentRect().w - 23, 0}, "%s",
                                utils::sepNumber(*seqNum, ',').c_str());
            this->_stylist().statusViewStd(*this);
        }

        this->_drawOffset();
    }

    const auto& dsFileState = _state->activeDataStreamFileState();
    const auto& path = dsFileState.dataStreamFile().path();
    const auto pathMaxLen = this->contentRect().w - 49;
    std::string dirNameStr, filenameStr;

    std::tie(dirNameStr, filenameStr) = utils::formatPath(path, pathMaxLen);
    this->_moveCursor({0, 0});

    if (!dirNameStr.empty()) {
        this->_print("%s/", dirNameStr.c_str());
    }

    this->_stylist().statusViewFilename(*this);
    this->_print("%s", filenameStr.c_str());
}

} // namespace jacques
