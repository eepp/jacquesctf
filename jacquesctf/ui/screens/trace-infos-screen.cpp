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

#include "config.hpp"
#include "trace-infos-screen.hpp"
#include "trace-infos-view.hpp"
#include "stylist.hpp"
#include "state.hpp"

namespace jacques {

TraceInfosScreen::TraceInfosScreen(const Rectangle& rect, const Config& cfg,
                                   std::shared_ptr<const Stylist> stylist,
                                   std::shared_ptr<State> state) :
    Screen {rect, cfg, stylist, state},
    _view {std::make_unique<TraceInfosView>(rect, stylist, state)}
{
    _view->focus();
}

void TraceInfosScreen::_redraw()
{
    _view->redraw();
}

void TraceInfosScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

void TraceInfosScreen::_visibilityChanged()
{
    _view->isVisible(this->isVisible());

    if (this->isVisible()) {
        _view->redraw();
    }
}

KeyHandlingReaction TraceInfosScreen::_handleKey(const int key)
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
        this->_state().gotoPreviousDataStreamFile();
        break;

    case KEY_F(4):
        this->_state().gotoNextDataStreamFile();
        break;

    default:
        break;
    }

    _view->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
