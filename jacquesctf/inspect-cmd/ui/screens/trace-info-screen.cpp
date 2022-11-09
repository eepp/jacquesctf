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
#include "trace-info-screen.hpp"
#include "../views/trace-info-view.hpp"
#include "../stylist.hpp"

namespace jacques {

TraceInfoScreen::TraceInfoScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                                 InspectCmdState& appState) :
    Screen {rect, cfg, stylist, appState},
    _view {std::make_unique<TraceInfoView>(rect, stylist, appState)}
{
    _view->focus();
}

void TraceInfoScreen::_redraw()
{
    _view->redraw();
}

void TraceInfoScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

void TraceInfoScreen::_visibilityChanged()
{
    _view->isVisible(this->isVisible());

    if (this->isVisible()) {
        _view->redraw();
    }
}

KeyHandlingReaction TraceInfoScreen::_handleKey(const int key)
{
    switch (key) {
    case KEY_UP:
        _view->prev();
        break;

    case KEY_DOWN:
        _view->next();
        break;
    case KEY_PPAGE:
        _view->pageUp();
        break;

    case  KEY_NPAGE:
        _view->pageDown();
        break;

    case KEY_F(3):
        this->_appState().gotoPrevDsFile();
        break;

    case KEY_F(4):
        this->_appState().gotoNextDsFile();
        break;

    default:
        break;
    }

    _view->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
