/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "packet-decoding-error-details-view.hpp"
#include "utils.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "active-packet-changed-message.hpp"

namespace jacques {

PacketDecodingErrorDetailsView::PacketDecodingErrorDetailsView(const Rectangle& rect,
                                                               std::shared_ptr<const Stylist> stylist,
                                                               std::shared_ptr<State> state) :
    View {rect, "", DecorationStyle::BORDERLESS, stylist},
    _state {state},
    _stateObserverGuard {*_state, *this}
{
}

void PacketDecodingErrorDetailsView::_redrawContent()
{
    if (!_state->hasActivePacketState() || !_state->activePacketState().packet().error()) {
        return;
    }

    const auto& error = *_state->activePacketState().packet().error();

    this->_stylist().packetDecodingErrorDetailsView(*this);
    this->_clearRect();

    // title
    this->_stylist().packetDecodingErrorDetailsView(*this, true);
    this->_moveAndPrint({1, 1}, "PACKET DECODING ERROR");

    // packet
    this->_stylist().packetDecodingErrorDetailsView(*this, true);
    this->_moveAndPrint({1, 3}, "Packet index:               ");
    this->_stylist().packetDecodingErrorDetailsView(*this, false);

    auto& packetIndexEntry = error.packetIndexEntry();
    auto str = utils::sepNumber(static_cast<long long>(packetIndexEntry.natIndexInDataStream()), ',');

    this->_print("%s", str.c_str());

    // offset in data stream file
    this->_stylist().packetDecodingErrorDetailsView(*this, true);
    this->_moveAndPrint({1, 4}, "Offset in data stream file: ");
    this->_stylist().packetDecodingErrorDetailsView(*this, false);

    auto sizeUnit = utils::formatSize(error.decodingError().offset(),
                                      utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                      ',');

    this->_print("%s %s", sizeUnit.first.c_str(), sizeUnit.second.c_str());
    sizeUnit = utils::formatSize(error.decodingError().offset(),
                                 utils::SizeFormatMode::BITS, ',');
    this->_print(" (%s %s)", sizeUnit.first.c_str(), sizeUnit.second.c_str());

    // offset in packet
    this->_stylist().packetDecodingErrorDetailsView(*this, true);
    this->_moveAndPrint({1, 5}, "Offset in packet:           ");
    this->_stylist().packetDecodingErrorDetailsView(*this, false);

    const auto offsetInPacketBits = error.decodingError().offset() -
                                    packetIndexEntry.offsetInDataStreamBits();

    sizeUnit = utils::formatSize(offsetInPacketBits,
                                 utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                 ',');
    this->_print("%s %s", sizeUnit.first.c_str(), sizeUnit.second.c_str());
    sizeUnit = utils::formatSize(offsetInPacketBits,
                                 utils::SizeFormatMode::BITS, ',');
    this->_print(" (%s %s)", sizeUnit.first.c_str(), sizeUnit.second.c_str());

    // message
    const auto msg = utils::wrapText(error.decodingError().reason(),
                                     this->contentRect().w - 2);
    auto it = std::begin(msg);
    Index curY = 7;
    Index curX = 1;

    this->_stylist().packetDecodingErrorDetailsView(*this, false);

    while (it != std::end(msg)) {
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

void PacketDecodingErrorDetailsView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        this->_redrawContent();
    }
}

} // namespace jacques
