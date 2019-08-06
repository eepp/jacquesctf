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
#include "packet-region-info-view.hpp"
#include "utils.hpp"
#include "message.hpp"
#include "content-packet-region.hpp"
#include "padding-packet-region.hpp"
#include "error-packet-region.hpp"

namespace jacques {

PacketRegionInfoView::PacketRegionInfoView(const Rectangle& rect,
                                           const Stylist& stylist,
                                           State& state) :
    View {rect, "Packet region info", DecorationStyle::BORDERLESS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
}

void PacketRegionInfoView::_stateChanged(const Message)
{
    this->redraw();
}

void PacketRegionInfoView::_safePrintScope(const yactfr::Scope scope)
{
    switch (scope) {
    case yactfr::Scope::PACKET_HEADER:
        this->_safePrint("PH");
        break;

    case yactfr::Scope::PACKET_CONTEXT:
        this->_safePrint("PC");
        break;

    case yactfr::Scope::EVENT_RECORD_HEADER:
        this->_safePrint("ERH");
        break;

    case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
        this->_safePrint("ER1C");
        break;

    case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
        this->_safePrint("ER2C");
        break;

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        this->_safePrint("ERP");
        break;
    }
}

void PacketRegionInfoView::_redrawContent()
{
    // clear
    this->_stylist().packetRegionInfoViewStd(*this);
    this->_clearRect();

    const auto packetRegion = _state->currentPacketRegion();

    if (!packetRegion) {
        return;
    }

    const ContentPacketRegion *cPacketRegion = nullptr;
    bool isError = false;

    this->_moveCursor({0, 0});

    if ((cPacketRegion = dynamic_cast<const ContentPacketRegion *>(packetRegion))) {
        // path
        const auto& path = _state->metadata().dataTypePath(cPacketRegion->dataType());

        if (path.path.empty()) {
            this->_stylist().packetRegionInfoViewStd(*this, true);
        }

        this->_safePrintScope(path.scope);

        if (path.path.empty()) {
            this->_stylist().packetRegionInfoViewStd(*this);
        }

        for (auto it = std::begin(path.path); it != std::end(path.path); ++it) {
            this->_safePrint("/");

            if (it == std::end(path.path) - 1) {
                this->_stylist().packetRegionInfoViewStd(*this, true);
            }

            this->_safePrint("%s", utils::escapeString(*it).c_str());
        }
    } else if (const auto sPacketRegion = dynamic_cast<const PaddingPacketRegion *>(packetRegion)) {
        if (packetRegion->scope()) {
            this->_stylist().packetRegionInfoViewStd(*this);
            this->_safePrintScope(packetRegion->scope()->scope());
            this->_print(" ");
        }

        this->_stylist().packetRegionInfoViewStd(*this, true);
        this->_print("PADDING");
    } else if (const auto sPacketRegion = dynamic_cast<const ErrorPacketRegion *>(packetRegion)) {
        this->_stylist().packetRegionInfoViewStd(*this, true);
        this->_print("ERROR");
        isError = true;
    }

    // size
    this->_stylist().packetRegionInfoViewStd(*this, false);

    const auto pathWidth = _state->activeDataStreamFileState().metadata().maxDataTypePathSize();
    const auto str = utils::sepNumber(packetRegion->segment().size()->bits(), ',');

    this->_safeMoveAndPrint({pathWidth + 4 + this->_curMaxOffsetSize() -
                             2 - str.size(), 0}, "%s b", str.c_str());

    // byte order
    if (packetRegion->segment().byteOrder()) {
        this->_safePrint("    ");

        if (*packetRegion->segment().byteOrder() == ByteOrder::BIG) {
            this->_safePrint("BE");
        } else {
            this->_safePrint("LE");
        }
    } else {
        this->_safePrint("      ");
    }

    // value
    if (cPacketRegion && cPacketRegion->value()) {
        const auto& varVal = *cPacketRegion->value();

        this->_safePrint("    ");
        this->_stylist().packetRegionInfoViewValue(*this);

        if (const auto val = boost::get<std::int64_t>(&varVal)) {
            this->_safePrint("%s", utils::sepNumber(static_cast<long long>(*val), ',').c_str());
        } else if (const auto val = boost::get<std::uint64_t>(&varVal)) {
            std::string intFmt;

            switch (cPacketRegion->dataType().asIntType()->displayBase()) {
            case yactfr::DisplayBase::OCTAL:
                intFmt = "0%" PRIo64;
                break;

            case yactfr::DisplayBase::HEXADECIMAL:
                intFmt = "0x%" PRIx64;
                break;

            default:
                break;
            }

            if (intFmt.empty()) {
                this->_safePrint("%s", utils::sepNumber(static_cast<unsigned long long>(*val), ',').c_str());
            } else {
                this->_safePrint(intFmt.c_str(), *val);
            }
        } else if (const auto val = boost::get<double>(&varVal)) {
            this->_safePrint("%f", *val);
        } else if (const auto val = boost::get<std::string>(&varVal)) {
            this->_safePrint("%s", utils::escapeString(*val).c_str());
        }
    } else if (isError) {
        const auto& error = _state->activePacketState().packet().error();

        assert(error);
        this->_safePrint("    ");
        this->_stylist().packetRegionInfoViewError(*this);
        this->_safePrint("%s", utils::escapeString(error->decodingError().what()).c_str());
    }
}

Size PacketRegionInfoView::_curMaxOffsetSize()
{
    assert(_state->hasActivePacketState());

    const auto& packet = _state->activePacketState().packet();
    const auto it = _maxOffsetSizes.find(&packet);

    if (it == std::end(_maxOffsetSizes)) {
        const auto str = utils::sepNumber(packet.indexEntry().effectiveTotalSize().bits());
        const auto size = str.size();

        _maxOffsetSizes[&packet] = size;
        return size;
    } else {
        return it->second;
    }
}

} // namespace jacques
