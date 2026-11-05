/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "pkt-decoding-error-details-view.hpp"
#include "utils.hpp"
#include "../stylist.hpp"
#include "../../state/msg.hpp"

namespace jacques {

PktDecodingErrorDetailsView::PktDecodingErrorDetailsView(const Rect& rect, const Stylist& stylist,
                                                         InspectCmdState& appState) :
    View {rect, "", DecorationStyle::BORDERLESS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
}

void PktDecodingErrorDetailsView::_redrawContent()
{
    if (!_appState->hasActivePktState() || !_appState->activePktState().pkt().error()) {
        return;
    }

    const auto& error = *_appState->activePktState().pkt().error();

    this->_style(Stylist::Role::ALERT);
    this->_clearRect();

    // title
    this->_style(Stylist::Role::ALERT, true);
    this->_moveAndPrint({1, 1}, "PACKET DECODING ERROR");

    // packet
    this->_style(Stylist::Role::ALERT, true);
    this->_moveAndPrint({1, 3}, "Packet index:               ");
    this->_style(Stylist::Role::ALERT);

    auto& pktIndexEntry = error.pktIndexEntry();
    auto str = utils::sepNumber(static_cast<long long>(pktIndexEntry.natIndexInDsFile()), ',');

    this->_print("%s", str.c_str());

    // offset in data stream file
    this->_style(Stylist::Role::ALERT, true);
    this->_moveAndPrint({1, 4}, "Offset in data stream file: ");
    this->_style(Stylist::Role::ALERT);

    {
        const auto [len, unit] = utils::formatLen(error.decodingError().offset(),
                                                  utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                  ',');

        this->_print("%s %s", len.c_str(), unit.c_str());
    }

    {
        const auto [len, unit] = utils::formatLen(error.decodingError().offset(),
                                                  utils::LenFmtMode::BITS, ',');

        this->_print(" (%s %s)", len.c_str(), unit.c_str());
    }

    // offset in packet
    this->_style(Stylist::Role::ALERT, true);
    this->_moveAndPrint({1, 5}, "Offset in packet:           ");
    this->_style(Stylist::Role::ALERT);

    const auto offsetInPktBits = error.decodingError().offset() -
                                 pktIndexEntry.offsetInDsFileBits();

    {
        const auto [len, unit] = utils::formatLen(offsetInPktBits,
                                                  utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                  ',');

        this->_print("%s %s", len.c_str(), unit.c_str());
    }

    {
        const auto [len, unit] = utils::formatLen(offsetInPktBits, utils::LenFmtMode::BITS, ',');

        this->_print(" (%s %s)", len.c_str(), unit.c_str());
    }

    // message
    const auto msg = utils::wrapText(utils::escapeStr(error.decodingError().reason()),
                                     this->contentRect().w - 2);
    auto it = msg.begin();
    Index curY = 7;
    Index curX = 1;

    this->_style(Stylist::Role::ALERT);

    while (it != msg.end()) {
        if (*it == '\n') {
            ++curY;
            ++it;
            curX = 1;
            continue;
        }

        this->_putChar({curX, curY}, *it);
        ++curX;
        ++it;
    }
}

void PktDecodingErrorDetailsView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_PKT_CHANGED) {
        this->_invalidate();
    }
}

} // namespace jacques
