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
#include "../views/ds-file-table-view.hpp"
#include "ds-files-screen.hpp"
#include "../../state/state.hpp"
#include "../stylist.hpp"

namespace jacques {

DsFilesScreen::DsFilesScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                             State& state) :
    Screen {rect, cfg, stylist, state},
    _view {std::make_unique<DsFileTableView>(rect, stylist, state)},
    _tsFmtModeWheel {
        TsFmtMode::LONG,
        TsFmtMode::NS_FROM_ORIGIN,
        TsFmtMode::CYCLES,
    },
    _dataLenFmtModeWheel {
        utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BITS,
    }
{
    _view->focus();
}

void DsFilesScreen::_redraw()
{
    _view->redraw();
}

void DsFilesScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

void DsFilesScreen::_visibilityChanged()
{
    _view->isVisible(this->isVisible());

    if (this->isVisible()) {
        _view->redraw();
    }
}

KeyHandlingReaction DsFilesScreen::_handleKey(const int key)
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

    case KEY_NPAGE:
        _view->pageDown();
        break;

    case KEY_END:
        _view->selectLast();
        break;

    case KEY_HOME:
        _view->selectFirst();
        break;

    case 'c':
        _view->centerSelRow();
        break;

    case 't':
        _tsFmtModeWheel.next();
        _view->tsFmtMode(_tsFmtModeWheel.curVal());
        break;

    case 's':
        _dataLenFmtModeWheel.next();
        _view->dataLenFmtMode(_dataLenFmtModeWheel.curVal());
        break;

    case '\n':
    case '\r':
        this->_state().gotoDsFile(_view->selDsFileIndex());

        if (this->_state().activeDsFileState().dsFile().pktCount() == 0) {
            break;
        } else if (this->_state().activeDsFileState().dsFile().pktCount() == 1) {
            return KeyHandlingReaction::RETURN_TO_INSPECT;
        } else {
            return KeyHandlingReaction::RETURN_TO_PKTS;
        }

    case KEY_F(3):
        this->_state().gotoPrevDsFile();
        break;

    case KEY_F(4):
        this->_state().gotoNextDsFile();
        break;

    default:
        break;
    }

    _view->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
