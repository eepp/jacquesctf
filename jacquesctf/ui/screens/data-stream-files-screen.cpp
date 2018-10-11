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
#include "data-stream-file-table-view.hpp"
#include "data-stream-files-screen.hpp"
#include "state.hpp"
#include "stylist.hpp"

namespace jacques {

DataStreamFilesScreen::DataStreamFilesScreen(const Rectangle& rect, const Config& cfg,
                                             std::shared_ptr<const Stylist> stylist,
                                             std::shared_ptr<State> state) :
    Screen {rect, cfg, stylist, state},
    _view {std::make_unique<DataStreamFileTableView>(rect, stylist, state)},
    _tsFormatModeWheel {
        TimestampFormatMode::LONG,
        TimestampFormatMode::NS_FROM_ORIGIN,
        TimestampFormatMode::CYCLES,
    },
    _dsFormatModeWheel {
        utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
        utils::SizeFormatMode::BYTES_FLOOR_WITH_EXTRA_BITS,
        utils::SizeFormatMode::BITS,
    }
{
    _view->focus();
}

void DataStreamFilesScreen::_redraw()
{
    _view->redraw();
}

void DataStreamFilesScreen::_resized()
{
    _view->moveAndResize(this->rect());
}

void DataStreamFilesScreen::_visibilityChanged()
{
    _view->isVisible(this->isVisible());

    if (this->isVisible()) {
        _view->redraw();
    }
}

KeyHandlingReaction DataStreamFilesScreen::_handleKey(const int key)
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
        _view->centerSelectedRow();
        break;

    case 't':
        _tsFormatModeWheel.next();
        _view->timestampFormatMode(_tsFormatModeWheel.currentValue());
        break;

    case 's':
        _dsFormatModeWheel.next();
        _view->dataSizeFormatMode(_dsFormatModeWheel.currentValue());
        break;

    case '\n':
    case '\r':
        this->_state().gotoDataStreamFile(_view->selectedDataStreamFileIndex());

        if (this->_state().activeDataStreamFileState().dataStreamFile().packetCount() == 0) {
            break;
        } else if (this->_state().activeDataStreamFileState().dataStreamFile().packetCount() == 1) {
            return KeyHandlingReaction::RETURN_TO_INSPECT;
        } else {
            return KeyHandlingReaction::RETURN_TO_PACKETS;
        }

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
