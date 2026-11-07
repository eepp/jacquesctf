/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP

#include "../stylist.hpp"
#include "screen.hpp"
#include "../views/help-view.hpp"

namespace jacques {

class HelpScreen final :
    public Screen
{
public:
    explicit HelpScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                        ScreenHost& host);

private:
    HelpView _view;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP
