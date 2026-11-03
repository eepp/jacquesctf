/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iostream>
#include <signal.h>
#include <unistd.h>

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
    this->_addView(*_view);
    _view->focus();
}

void TraceInfoScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

KeyHandlingReaction TraceInfoScreen::_handleKey(const Key key)
{
    if (const auto specialKey = key.special()) {
        switch (*specialKey) {
        case SpecialKey::UP:
            _view->prev();
            break;

        case SpecialKey::DOWN:
            _view->next();
            break;

        case SpecialKey::PAGE_UP:
            _view->pageUp();
            break;

        case SpecialKey::PAGE_DOWN:
            _view->pageDown();
            break;

        case SpecialKey::F3:
            this->_appState().gotoPrevDsFile();
            break;

        case SpecialKey::F4:
            this->_appState().gotoNextDsFile();
            break;

        default:
            break;
        }
    }

    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
