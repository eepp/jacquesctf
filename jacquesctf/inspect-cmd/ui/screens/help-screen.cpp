/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

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
    _view->focus();
}

void HelpScreen::_redraw()
{
    _view->redraw();
}

void HelpScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

void HelpScreen::_visibilityChanged()
{
    _view->isVisible(this->isVisible());

    if (this->isVisible()) {
        _view->redraw();
    }
}

KeyHandlingReaction HelpScreen::_handleKey(const int key)
{
    if (key == KEY_UP) {
        _view->prev();
    } else if (key == KEY_DOWN) {
        _view->next();
    } else if (key == KEY_PPAGE) {
        _view->pageUp();
    } else if (key == KEY_NPAGE) {
        _view->pageDown();
    }

    _view->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
