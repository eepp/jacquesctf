/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "../key.hpp"
#include "cfg.hpp"
#include "../views/ds-file-table-view.hpp"
#include "ds-files-screen.hpp"
#include "../stylist.hpp"

namespace jacques {

DsFilesScreen::DsFilesScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                             InspectCmdState& appState) :
    Screen {rect, cfg, stylist, appState},
    _view {std::make_unique<DsFileTableView>(rect, stylist, appState)},
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
    _view->selectFirst();
    this->_addView(*_view);
    _view->focus();
}

void DsFilesScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

KeyHandlingReaction DsFilesScreen::_handleKey(const Key key)
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

        case SpecialKey::END:
            _view->selectLast();
            break;

        case SpecialKey::HOME:
            _view->selectFirst();
            break;

        case SpecialKey::ENTER:
            this->_appState().gotoDsFile(_view->selDsFileIndex());

            if (this->_appState().activeDsFileState().dsFile().pktCount() == 0) {
                break;
            } else if (this->_appState().activeDsFileState().dsFile().pktCount() == 1) {
                return KeyHandlingReaction::RETURN_TO_INSPECT;
            } else {
                return KeyHandlingReaction::RETURN_TO_PKTS;
            }

        case SpecialKey::F3:
            this->_appState().gotoPrevDsFile();
            break;

        case SpecialKey::F4:
            this->_appState().gotoNextDsFile();
            break;

        default:
            break;
        }
    } else {
        const auto ch = *key.ch();

        switch (ch) {
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

        default:
            break;
        }
    }

    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
