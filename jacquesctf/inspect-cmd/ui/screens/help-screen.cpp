/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "../key.hpp"
#include "cfg.hpp"
#include "help-screen.hpp"
#include "../views/help-view.hpp"
#include "../stylist.hpp"

namespace jacques {

HelpScreen::HelpScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                       InspectCmdState& appState) :
    Screen {rect, cfg, stylist, appState},
    _view {std::make_unique<HelpView>(rect, stylist)}
{
    this->_addView(*_view);
    _view->focus();
}

void HelpScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

KeyHandlingReaction HelpScreen::_handleKey(const Key key)
{
    if (key.special() == SpecialKey::UP) {
        _view->prev();
    } else if (key.special() == SpecialKey::DOWN) {
        _view->next();
    } else if (key.special() == SpecialKey::PAGE_UP) {
        _view->pageUp();
    } else if (key.special() == SpecialKey::PAGE_DOWN) {
        _view->pageDown();
    }

    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
