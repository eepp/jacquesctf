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
    ViewInspectCmdStateObserver {appState, *this}
{
}

void PktDecodingErrorDetailsView::_redrawContent()
{
    if (!this->_appState().hasActivePktState() || !this->_appState().activePktState().pkt().error()) {
        return;
    }

    const auto& error = *this->_appState().activePktState().pkt().error();
    const auto& pktIndexEntry = error.pktIndexEntry();

    this->_clearContent(Stylist::Role::ALERT);

    // title
    this->_style(Stylist::Role::ALERT, true);
    this->_write({1, 1}, "PACKET DECODING ERROR");

    // packet index and offsets
    const auto offsetStr = [](const Size offsetBits) {
        const auto [len, unit] = utils::formatLen(offsetBits,
                                                  utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                  ',');
        const auto [lenBits, unitBits] = utils::formatLen(offsetBits, utils::LenFmtMode::BITS,
                                                          ',');

        return len + ' ' + unit + " (" + lenBits + ' ' + unitBits + ')';
    };

    this->_writeLabelValRows({1, 3}, {
        {"Packet index:", utils::sepNumber(pktIndexEntry.natIndexInDsFile(), ',')},
        {"Offset in data stream file:", offsetStr(error.decodingError().offset())},
        {
            "Offset in packet:",
            offsetStr(error.decodingError().offset() - pktIndexEntry.offsetInDsFileBits())
        },
    }, Stylist::Role::ALERT, true);

    // message
    const auto lines = utils::wrapTextLines(utils::escapeStr(error.decodingError().reason()),
                                            this->contentRect().w - 2);
    Index y = 7;

    this->_style(Stylist::Role::ALERT);

    for (auto& line : lines) {
        this->_write({1, y}, line);
        ++y;
    }
}

void PktDecodingErrorDetailsView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED || msg == Message::ACTIVE_PKT_CHANGED) {
        this->_invalidate();
    }
}

} // namespace jacques
