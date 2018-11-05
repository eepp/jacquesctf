/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cinttypes>
#include <algorithm>
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
                       const Stylist& stylist, State& state) :
    View {rect, "Status", DecorationStyle::BORDERLESS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_createEndPositions();
}

void StatusView::_createEndPositions()
{
    for (const auto& dsfState : _state->dataStreamFileStates()) {
        _EndPositions positions;
        const auto& dsf = dsfState->dataStreamFile();
        const auto packetCountStr = utils::sepNumber(dsf.packetCount());

        positions.packetCount = 0;
        positions.packetIndex = positions.packetCount + packetCountStr.size() + 1;
        positions.seqNum = positions.packetIndex + packetCountStr.size() + 5;
        positions.curOffsetInPacketBits = positions.seqNum +
                                          packetCountStr.size() + 6;

        const auto& maxEntryIt = std::max_element(std::begin(dsf.packetIndexEntries()),
                                                  std::end(dsf.packetIndexEntries()),
                                                  [](const auto& entryA,
                                                     const auto& entryB) {
            return entryA.effectiveTotalSize() < entryB.effectiveTotalSize();
        });
        const auto maxOffsetInPacketBitsStr = utils::sepNumber(maxEntryIt->effectiveTotalSize().bits());

        positions.dsfPath = positions.curOffsetInPacketBits +
                            maxOffsetInPacketBitsStr.size() + 6;
        _endPositions[dsfState.get()] = positions;
    }
}

void StatusView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg) ||
            dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        _curEndPositions = &_endPositions[&_state->activeDataStreamFileState()];
        this->redraw();
    } else if (dynamic_cast<const CurOffsetInPacketChangedMessage *>(&msg)) {
        this->_drawOffset();
    }
}

void StatusView::_drawOffset()
{
    if (!_curEndPositions || !_state->hasActivePacketState()) {
        return;
    }

    // clear previous
    this->_stylist().statusViewStd(*this);

    for (auto x = this->contentRect().w - _curEndPositions->dsfPath;
            x < this->contentRect().w - _curEndPositions->curOffsetInPacketBits; ++x) {
        this->_putChar({x, 0}, ' ');
    }

    // draw new
    this->_stylist().statusViewStd(*this, true);

    const auto str = utils::sepNumber(_state->activePacketState().curOffsetInPacketBits(),
                                      ',');

    this->_moveAndPrint({this->contentRect().w -
                         _curEndPositions->curOffsetInPacketBits - 2 - str.size(),
                         0}, "%s", str.c_str());
    this->_stylist().statusViewStd(*this);
    this->_print(" b");
}

void StatusView::_redrawContent()
{
    // clear
    this->_stylist().statusViewStd(*this);
    this->_clearRect();

    if (!_curEndPositions) {
        return;
    }

    if (_state->hasActivePacketState()) {
        // packet index and count
        const auto count = _state->activeDataStreamFileState().dataStreamFile().packetCount();
        const auto countStr = utils::sepNumber(count, ',');
        const auto index = _state->activePacketState().packetIndexEntry().natIndexInDataStream();
        const auto indexStr = utils::sepNumber(index, ',');

        this->_putChar({this->contentRect().w - _curEndPositions->packetIndex -
                        indexStr.size() - 1, 0}, '#');
        this->_stylist().statusViewStd(*this, true);
        this->_print("%s", indexStr.c_str());
        this->_stylist().statusViewStd(*this);
        this->_print("/%s", countStr.c_str());

        // packet sequence number
        const auto& seqNum = _state->activePacketState().packetIndexEntry().seqNum();

        if (seqNum) {
            const auto seqNumStr = utils::sepNumber(*seqNum, ',');

            this->_moveAndPrint({this->contentRect().w - _curEndPositions->seqNum -
                                 seqNumStr.size() - 2, 0}, "##");
            this->_stylist().statusViewStd(*this, true);
            this->_safePrint("%s", seqNumStr.c_str());
        }
    }

    this->_drawOffset();

    const auto& path = _state->activeDataStreamFileState().dataStreamFile().path();
    const auto pathMaxLen = this->contentRect().w - _curEndPositions->dsfPath;
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
