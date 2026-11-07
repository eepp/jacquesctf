/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "help-screen.hpp"

namespace jacques {

HelpScreen::HelpScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                       ScreenHost& host) :
    Screen {rect, stylist, appState, host},
    _view {rect, stylist}
{
    this->_addMainView(_view);
}

} // namespace jacques
