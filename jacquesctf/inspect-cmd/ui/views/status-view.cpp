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

#include "../stylist.hpp"
#include "status-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"

namespace jacques {

StatusView::StatusView(const Rect& rect, const Stylist& stylist, State& state) :
    View {rect, "Status", DecorationStyle::BORDERLESS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_createEndPositions();
}

void StatusView::_createEndPositions()
{
    for (const auto& dsfState : _state->dsFileStates()) {
        _EndPositions positions;
        const auto& dsf = dsfState->dsFile();
        const auto pktCountStr = utils::sepNumber(dsf.pktCount());

        positions.pktCount = 0;
        positions.pktIndex = positions.pktCount + pktCountStr.size() + 1;
        positions.seqNum = positions.pktIndex + pktCountStr.size() + 5;
        positions.pktPercent = positions.seqNum + pktCountStr.size() + 6;
        positions.curOffsetInDsFileBits = positions.pktPercent + 9;
        positions.curOffsetInPktBits = positions.curOffsetInDsFileBits + 17;

        const auto maxEntryIt = std::max_element(dsf.pktIndexEntries().begin(),
                                                 dsf.pktIndexEntries().end(),
                                                 [](const auto& entryA, const auto& entryB) {
            return entryA.effectiveTotalLen() < entryB.effectiveTotalLen();
        });
        const auto maxOffsetInPktBitsStr = (maxEntryIt == dsf.pktIndexEntries().end()) ?
                                           std::string {} :
                                           utils::sepNumber(maxEntryIt->effectiveTotalLen().bits());

        positions.dsfPath = positions.curOffsetInPktBits + maxOffsetInPktBitsStr.size() + 6;
        _endPositions[dsfState.get()] = positions;
    }
}

void StatusView::_stateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_CHANGED || msg == Message::ACTIVE_PKT_CHANGED) {
        _curEndPositions = &_endPositions[&_state->activeDsFileState()];
        this->redraw();
    } else if (msg == Message::CUR_OFFSET_IN_PKT_CHANGED) {
        this->_drawOffset();
        this->refresh();
    }
}

void StatusView::_drawOffset()
{
    if (!_curEndPositions || !_state->hasActivePktState()) {
        return;
    }

    // clear previous
    this->_stylist().statusViewStd(*this);

    for (auto x = this->contentRect().w - _curEndPositions->dsfPath;
            x < this->contentRect().w - _curEndPositions->pktPercent; ++x) {
        this->_putChar({x, 0}, ' ');
    }

    // draw percentage
    this->_stylist().statusViewStd(*this, true);

    const auto percent = _state->activePktState().curOffsetInPktBits() * 100 /
                         _state->activePktState().pkt().indexEntry().effectiveTotalLen().bits();

    this->_moveAndPrint({this->contentRect().w - _curEndPositions->pktPercent - 5, 0}, "%3u",
                        static_cast<unsigned int>(percent));
    this->_stylist().statusViewStd(*this);
    this->_print(" %%");

    // draw new offset in packet
    this->_stylist().statusViewStd(*this, true);

    auto str = utils::sepNumber(_state->activePktState().curOffsetInPktBits(), ',');

    this->_moveAndPrint({
        this->contentRect().w - _curEndPositions->curOffsetInPktBits - 2 - str.size(), 0
    }, "%s", str.c_str());
    this->_stylist().statusViewStd(*this);
    this->_print(" b");

    // draw new offset in data stream file
    str = utils::sepNumber(_state->activeDsFileState().curOffsetInDsFileBits(), ',');
    this->_moveAndPrint({
        this->contentRect().w - _curEndPositions->curOffsetInDsFileBits - str.size() - 3, 0
    }, "(", str.c_str());
    this->_stylist().statusViewStd(*this);
    this->_print(str.c_str());
    this->_stylist().statusViewStd(*this);
    this->_print(" b)");
}

void StatusView::_redrawContent()
{
    // clear
    this->_stylist().statusViewStd(*this);
    this->_clearRect();

    if (!_curEndPositions) {
        return;
    }

    if (_state->hasActivePktState()) {
        // packet index and count
        const auto count = _state->activeDsFileState().dsFile().pktCount();
        const auto countStr = utils::sepNumber(count, ',');
        const auto index = _state->activePktState().pktIndexEntry().natIndexInDsFile();
        const auto indexStr = utils::sepNumber(index, ',');

        this->_putChar({
            this->contentRect().w - _curEndPositions->pktIndex -
            indexStr.size() - 1, 0
        }, '#');
        this->_stylist().statusViewStd(*this, true);
        this->_print("%s", indexStr.c_str());
        this->_stylist().statusViewStd(*this);
        this->_print("/%s", countStr.c_str());

        // packet sequence number
        const auto& seqNum = _state->activePktState().pktIndexEntry().seqNum();

        if (seqNum) {
            const auto seqNumStr = utils::sepNumber(*seqNum, ',');

            this->_moveAndPrint({
                this->contentRect().w - _curEndPositions->seqNum - seqNumStr.size() - 2, 0
            }, "##");
            this->_stylist().statusViewStd(*this, true);
            this->_safePrint("%s", seqNumStr.c_str());
        }
    }

    this->_drawOffset();

    const auto& path = utils::escapeStr(_state->activeDsFileState().dsFile().path().string());
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
