/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "../key.hpp"
#include "cfg.hpp"
#include "help-screen.hpp"
#include "../views/help-view.hpp"
#include "../stylist.hpp"

namespace jacques {

HelpScreen::HelpScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                       InspectCmdState& appState, ScreenHost& host) :
    Screen {rect, cfg, stylist, appState, host},
    _view {std::make_unique<HelpView>(rect, stylist)}
{
    this->_addMainView(*_view);
}

} // namespace jacques
