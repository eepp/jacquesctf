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
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
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

void PacketRegionInfoView::_stateChanged(const Message& msg)
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

            this->_safePrint("%s", it->c_str());
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
    }

    // size
    this->_stylist().packetRegionInfoViewStd(*this, false);
    this->_safePrint("    %s b",
                     utils::sepNumber(packetRegion->segment().size().bits(), ',').c_str());

    // byte order
    if (packetRegion->segment().byteOrder()) {
        this->_safePrint("    ");

        if (*packetRegion->segment().byteOrder() == ByteOrder::BIG) {
            this->_safePrint("BE");
        } else {
            this->_safePrint("LE");
        }
    }

    // value
    if (cPacketRegion && cPacketRegion->value()) {
        const auto& varVal = *cPacketRegion->value();
        std::string intFmt;

        if (cPacketRegion->dataType().isIntType()) {
            const auto& intType = *cPacketRegion->dataType().asIntType();

            if (intType.isUnsignedIntType()) {
                switch (intType.displayBase()) {
                case yactfr::DisplayBase::OCTAL:
                    intFmt = "0%" PRIo64;
                    break;

                case yactfr::DisplayBase::HEXADECIMAL:
                    intFmt = "0x%" PRIx64;
                    break;

                default:
                    intFmt = "%" PRIu64;
                    break;
                }
            } else {
                intFmt = "%" PRId64;
            }
        }

        this->_safePrint("    ");
        this->_stylist().packetRegionInfoViewValue(*this);

        if (const auto val = boost::get<std::int64_t>(&varVal)) {
            assert(!intFmt.empty());
            this->_safePrint(intFmt.c_str(), *val);
        } else if (const auto val = boost::get<std::uint64_t>(&varVal)) {
            assert(!intFmt.empty());
            this->_safePrint(intFmt.c_str(), *val);
        } else if (const auto val = boost::get<double>(&varVal)) {
            this->_safePrint("%f", *val);
        } else if (const auto val = boost::get<std::string>(&varVal)) {
            this->_safePrint("%s", val->c_str());
        }
    }
}

} // namespace jacques
