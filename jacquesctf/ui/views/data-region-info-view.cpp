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
#include "data-region-info-view.hpp"
#include "utils.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
#include "content-data-region.hpp"
#include "padding-data-region.hpp"
#include "error-data-region.hpp"

namespace jacques {

DataRegionInfoView::DataRegionInfoView(const Rectangle& rect,
                                       std::shared_ptr<const Stylist> stylist,
                                       std::shared_ptr<State> state) :
    View {rect, "Data region info", DecorationStyle::BORDERLESS, stylist},
    _state {state},
    _stateObserverGuard {*state, *this}
{
}

void DataRegionInfoView::_stateChanged(const Message& msg)
{
    this->redraw();
}

void DataRegionInfoView::_safePrintScope(const yactfr::Scope scope)
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

void DataRegionInfoView::_redrawContent()
{
    // clear
    this->_stylist().dataRegionInfoViewStd(*this);
    this->_clearRect();

    const auto dataRegion = _state->currentDataRegion();

    if (!dataRegion) {
        return;
    }

    const ContentDataRegion *cDataRegion = nullptr;

    this->_moveCursor({0, 0});

    if ((cDataRegion = dynamic_cast<const ContentDataRegion *>(dataRegion))) {
        // path
        const auto& path = _state->metadata().dataTypePath(cDataRegion->dataType());

        if (path.path.empty()) {
            this->_stylist().dataRegionInfoViewStd(*this, true);
        }

        this->_safePrintScope(path.scope);

        if (path.path.empty()) {
            this->_stylist().dataRegionInfoViewStd(*this);
        }

        for (auto it = std::begin(path.path); it != std::end(path.path); ++it) {
            this->_safePrint("/");

            if (it == std::end(path.path) - 1) {
                this->_stylist().dataRegionInfoViewStd(*this, true);
            }

            this->_safePrint("%s", it->c_str());
        }
    } else if (const auto sDataRegion = dynamic_cast<const PaddingDataRegion *>(dataRegion)) {
        if (dataRegion->scope()) {
            this->_stylist().dataRegionInfoViewStd(*this);
            this->_safePrintScope(dataRegion->scope()->scope());
            this->_print(" ");
        }

        this->_stylist().dataRegionInfoViewStd(*this, true);
        this->_print("PADDING");
    } else if (const auto sDataRegion = dynamic_cast<const ErrorDataRegion *>(dataRegion)) {
        this->_stylist().dataRegionInfoViewStd(*this, true);
        this->_print("ERROR");
    }

    // size
    this->_stylist().dataRegionInfoViewStd(*this, false);
    this->_safePrint("    %s b",
                     utils::sepNumber(dataRegion->segment().size().bits(), ',').c_str());

    // byte order
    if (dataRegion->byteOrder()) {
        this->_safePrint("    ");

        if (*dataRegion->byteOrder() == ByteOrder::BIG) {
            this->_safePrint("BE");
        } else {
            this->_safePrint("LE");
        }
    }

    // value
    if (cDataRegion && cDataRegion->value()) {
        const auto& varVal = *cDataRegion->value();
        std::string intFmt;

        if (cDataRegion->dataType().isIntType()) {
            const auto& intType = *cDataRegion->dataType().asIntType();

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
        this->_stylist().dataRegionInfoViewValue(*this);

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
