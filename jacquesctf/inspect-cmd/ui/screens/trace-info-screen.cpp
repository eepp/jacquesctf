/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "../key.hpp"
#include "cfg.hpp"
#include "trace-info-screen.hpp"
#include "../views/trace-info-view.hpp"
#include "../stylist.hpp"

namespace jacques {

TraceInfoScreen::TraceInfoScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                                 InspectCmdState& appState) :
    Screen {rect, cfg, stylist, appState},
    _view {std::make_unique<TraceInfoView>(rect, stylist, appState)}
{
    this->_addMainView(*_view);
}

KeyHandlingReaction TraceInfoScreen::_handleKey(const Key key)
{
    this->_handleDsFileKey(key);
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
