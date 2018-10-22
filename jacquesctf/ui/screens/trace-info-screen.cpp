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
#include "trace-info-screen.hpp"
#include "trace-info-view.hpp"
#include "stylist.hpp"
#include "state.hpp"

namespace jacques {

TraceInfoScreen::TraceInfoScreen(const Rectangle& rect, const Config& cfg,
                                 const Stylist& stylist, State& state) :
    Screen {rect, cfg, stylist, state},
    _view {std::make_unique<TraceInfoView>(rect, stylist, state)}
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
