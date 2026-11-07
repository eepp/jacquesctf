/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>

#include "../stylist.hpp"
#include "status-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"

namespace jacques {

StatusView::StatusView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    View {rect, "Status", DecorationStyle::BORDERLESS, stylist},
    ViewInspectCmdStateObserver {appState, *this}
{
    this->_createEndPositions();
}

void StatusView::_createEndPositions()
{
    for (const auto& dsfState : this->_appState().dsFileStates()) {
        _EndPositions positions;
        const auto& dsf = dsfState->dsFile();
        const auto pktCountStr = utils::sepNumber(dsf.pktCount());
        const auto maxSeqNumEntryIt = std::max_element(dsf.pktIndexEntries().begin(),
                                                       dsf.pktIndexEntries().end(),
                                                       [](const auto& entryA, const auto& entryB) {
            return entryA.seqNum() < entryB.seqNum();
        });
        const auto maxSeqNumStr = (maxSeqNumEntryIt == dsf.pktIndexEntries().end() ||
                                   !maxSeqNumEntryIt->seqNum()) ?
                                  std::string {} :
                                  utils::sepNumber(*maxSeqNumEntryIt->seqNum());
        const auto fileLenBitsStr = utils::sepNumber(dsf.fileLen().bits());

        positions.pktCount = 0;
        positions.pktIndex = positions.pktCount + pktCountStr.size() + 1;
        positions.seqNum = positions.pktIndex + pktCountStr.size() + 5;
        positions.pktPercent = positions.seqNum + maxSeqNumStr.size() + 6;
        positions.curOffsetInDsFileBits = positions.pktPercent + 9;
        positions.curOffsetInPktBits = positions.curOffsetInDsFileBits +
                                       fileLenBitsStr.size() + 7;

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

void StatusView::_appStateChanged(Message)
{
    this->_invalidate();
}

void StatusView::_drawOffset(const _EndPositions& endPositions)
{
    if (!this->_appState().hasActivePktState()) {
        return;
    }

    // clear previous
    this->_style(Stylist::Role::STATUS);
    this->_fill({
        {this->contentRect().w - endPositions.dsfPath, 0},
        endPositions.dsfPath - endPositions.pktPercent, 1
    });

    // draw percentage
    this->_style(Stylist::Role::STATUS, true);

    const auto percent = this->_appState().activePktState().curOffsetInPktBits() * 100 /
                         this->_appState().activePktState().pkt().indexEntry().effectiveTotalLen().bits();
    auto x = this->_write({this->contentRect().w - endPositions.pktPercent - 5, 0},
                          utils::format("%3u", static_cast<unsigned int>(percent)));

    this->_style(Stylist::Role::STATUS);
    this->_write({x, 0}, " %");

    // draw new offset in packet
    this->_style(Stylist::Role::STATUS, true);

    auto str = utils::sepNumber(this->_appState().activePktState().curOffsetInPktBits(), ',');

    x = this->_write({
        this->contentRect().w - endPositions.curOffsetInPktBits - 2 - str.size(), 0
    }, str);
    this->_style(Stylist::Role::STATUS);
    this->_write({x, 0}, " b");

    // draw new offset in data stream file
    str = utils::sepNumber(this->_appState().activeDsFileState().curOffsetInDsFileBits(), ',');
    this->_write({
        this->contentRect().w - endPositions.curOffsetInDsFileBits - str.size() - 3, 0
    }, "(" + str + " b)");
}

void StatusView::_redrawContent()
{
    this->_clearContent(Stylist::Role::STATUS);

    const auto& endPositions = _endPositions.at(&this->_appState().activeDsFileState());

    if (this->_appState().hasActivePktState()) {
        // packet index and count
        const auto count = this->_appState().activeDsFileState().dsFile().pktCount();
        const auto countStr = utils::sepNumber(count, ',');
        const auto index = this->_appState().activePktState().pktIndexEntry().natIndexInDsFile();
        const auto indexStr = utils::sepNumber(index, ',');
        auto x = this->_write({
            this->contentRect().w - endPositions.pktIndex - indexStr.size() - 1, 0
        }, "#");

        this->_style(Stylist::Role::STATUS, true);
        x = this->_write({x, 0}, indexStr);
        this->_style(Stylist::Role::STATUS);
        this->_write({x, 0}, "/" + countStr);

        // packet sequence number
        const auto& seqNum = this->_appState().activePktState().pktIndexEntry().seqNum();

        if (seqNum) {
            const auto seqNumStr = utils::sepNumber(*seqNum, ',');

            x = this->_write({
                this->contentRect().w - endPositions.seqNum - seqNumStr.size() - 2, 0
            }, "##");
            this->_style(Stylist::Role::STATUS, true);
            this->_write({x, 0}, seqNumStr);
        }
    }

    this->_drawOffset(endPositions);

    const auto& path = utils::escapeStr(this->_appState().activeDsFileState().dsFile().path().string());
    const auto pathMaxLen = this->contentRect().w - endPositions.dsfPath;
    const auto [dirNameStr, filenameStr] = _formatPath(path, pathMaxLen);
    Index x = 0;

    this->_style(Stylist::Role::STATUS);

    if (!dirNameStr.empty()) {
        x = this->_write({x, 0}, dirNameStr + '/');
    }

    this->_style(Stylist::Role::STATUS, true);
    this->_write({x, 0}, filenameStr);
}

} // namespace jacques
