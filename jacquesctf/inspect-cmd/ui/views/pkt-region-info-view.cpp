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

#include "../stylist.hpp"
#include "pkt-region-info-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"
#include "data/content-pkt-region.hpp"
#include "data/padding-pkt-region.hpp"
#include "data/error-pkt-region.hpp"

namespace jacques {

PktRegionInfoView::PktRegionInfoView(const Rect& rect, const Stylist& stylist, State& state) :
    View {rect, "Packet region info", DecorationStyle::BORDERLESS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
}

void PktRegionInfoView::_stateChanged(const Message)
{
    this->redraw();
}

static const char *scopeStr(const yactfr::Scope scope) noexcept
{
    switch (scope) {
    case yactfr::Scope::PACKET_HEADER:
        return "PH";

    case yactfr::Scope::PACKET_CONTEXT:
        return "PC";

    case yactfr::Scope::EVENT_RECORD_HEADER:
        return "ERH";

    case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
        return "ER1C";

    case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
        return "ER2C";

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        return "ERP";

    default:
        std::abort();
    }
}

void PktRegionInfoView::_safePrintScope(const yactfr::Scope scope)
{
    this->_safePrint(scopeStr(scope));
}

void PktRegionInfoView::_redrawContent()
{
    // clear
    this->_stylist().pktRegionInfoViewStd(*this);
    this->_clearRect();

    const auto pktRegion = _state->curPktRegion();

    if (!pktRegion) {
        return;
    }

    const ContentPktRegion *cPktRegion = nullptr;
    bool isError = false;

    this->_moveCursor({0, 0});

    if ((cPktRegion = dynamic_cast<const ContentPktRegion *>(pktRegion))) {
        // path
        const auto& path = _state->metadata().dtPath(cPktRegion->dt());

        if (path.path.empty()) {
            this->_stylist().pktRegionInfoViewStd(*this, true);
        }

        this->_safePrintScope(path.scope);

        if (path.path.empty()) {
            this->_stylist().pktRegionInfoViewStd(*this);
        }

        for (auto it = path.path.begin(); it != path.path.end(); ++it) {
            this->_safePrint("/");

            if (it == path.path.end() - 1) {
                this->_stylist().pktRegionInfoViewStd(*this, true);
            }

            this->_safePrint("%s", utils::escapeStr(*it).c_str());
        }
    } else if (const auto sPktRegion = dynamic_cast<const PaddingPktRegion *>(pktRegion)) {
        if (pktRegion->scope()) {
            this->_stylist().pktRegionInfoViewStd(*this);
            this->_safePrintScope(pktRegion->scope()->scope());
            this->_print(" ");
        }

        this->_stylist().pktRegionInfoViewStd(*this, true);
        this->_print("PADDING");
    } else if (const auto sPktRegion = dynamic_cast<const ErrorPktRegion *>(pktRegion)) {
        this->_stylist().pktRegionInfoViewStd(*this, true);
        this->_print("ERROR");
        isError = true;
    }

    // size
    this->_stylist().pktRegionInfoViewStd(*this, false);

    const auto pathWidth = _state->activeDsFileState().metadata().maxDtPathSize();
    const auto str = utils::sepNumber(pktRegion->segment().len()->bits(), ',');

    this->_safeMoveAndPrint({
        pathWidth + 4 + this->_curMaxOffsetSize() - 2 - str.size(), 0
    }, "%s b", str.c_str());

    // byte order
    if (pktRegion->segment().bo()) {
        this->_safePrint("    ");

        if (*pktRegion->segment().bo() == Bo::BIG) {
            this->_safePrint("BE");
        } else {
            this->_safePrint("LE");
        }
    } else {
        this->_safePrint("      ");
    }

    // value
    if (cPktRegion && cPktRegion->val()) {
        const auto& varVal = *cPktRegion->val();

        this->_safePrint("    ");
        this->_stylist().pktRegionInfoViewVal(*this);

        if (const auto val = boost::get<std::int64_t>(&varVal)) {
            this->_safePrint("%s", utils::sepNumber(static_cast<long long>(*val), ',').c_str());
        } else if (const auto val = boost::get<std::uint64_t>(&varVal)) {
            std::string intFmt;

            switch (cPktRegion->dt().asIntType()->displayBase()) {
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
            this->_safePrint("%s", utils::escapeStr(*val).c_str());
        }
    } else if (isError) {
        const auto& error = _state->activePktState().pkt().error();

        assert(error);
        this->_safePrint("    ");
        this->_stylist().pktRegionInfoViewError(*this);
        this->_safePrint("%s", utils::escapeStr(error->decodingError().what()).c_str());
    }
}

Size PktRegionInfoView::_curMaxOffsetSize()
{
    assert(_state->hasActivePktState());

    const auto& pkt = _state->activePktState().pkt();
    const auto it = _maxOffsetSizes.find(&pkt);

    if (it == _maxOffsetSizes.end()) {
        const auto str = utils::sepNumber(pkt.indexEntry().effectiveTotalLen().bits());
        const auto size = str.size();

        _maxOffsetSizes[&pkt] = size;
        return size;
    } else {
        return it->second;
    }
}

} // namespace jacques
