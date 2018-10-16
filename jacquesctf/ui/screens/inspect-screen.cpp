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
#include "event-record-table-view.hpp"
#include "inspect-screen.hpp"
#include "stylist.hpp"
#include "state.hpp"

namespace jacques {

InspectScreen::InspectScreen(const Rectangle& rect, const Config& cfg,
                             std::shared_ptr<const Stylist> stylist,
                             std::shared_ptr<State> state) :
    Screen {rect, cfg, stylist, state},
    _decErrorView {
        std::make_unique<PacketDecodingErrorDetailsView>(rect, stylist, state)
    },
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
    Rectangle ertViewRect;
    Rectangle dtPathViewRect;

    std::tie(ertViewRect, dtPathViewRect) = this->_viewRects();
    _ertView = std::make_unique<EventRecordTableView>(ertViewRect, stylist,
                                                      state);
    _dtPathView = std::make_unique<DataTypePathView>(dtPathViewRect,
                                                     stylist, state);
    _decErrorView->isVisible(false);
    _ertView->focus();
}

std::tuple<Rectangle, Rectangle> InspectScreen::_viewRects() const
{
    return {
        {this->rect().pos, this->rect().w, this->rect().h - 1},
        {{this->rect().pos.x, this->rect().h - 1}, this->rect().w, 1}
    };
}

void InspectScreen::_redraw()
{
    _ertView->redraw();
    _dtPathView->redraw();
    _decErrorView->redraw();
}

void InspectScreen::_resized()
{
    Rectangle ertViewRect;
    Rectangle dtPathViewRect;

    std::tie(ertViewRect, dtPathViewRect) = this->_viewRects();
    _ertView->moveAndResize(ertViewRect);
    _dtPathView->moveAndResize(dtPathViewRect);
    _decErrorView->moveAndResize(Rectangle {{this->rect().pos.x + 4,
                                             this->rect().h - 14},
                                            this->rect().w - 8, 12});
    _ertView->centerSelectedRow(false);
}

void InspectScreen::_visibilityChanged()
{
    _ertView->isVisible(this->isVisible());
    _dtPathView->isVisible(this->isVisible());

    if (this->isVisible()) {
        _ertView->redraw();
        _dtPathView->redraw();
        this->_tryShowDecodingError();
        _decErrorView->refresh(true);
    }
}

void InspectScreen::_tryShowDecodingError()
{
    if (this->_state().hasActivePacket() &&
            this->_state().activePacket().error()) {
        _decErrorView->moveAndResize(Rectangle {{this->rect().pos.x + 4,
                                                 this->rect().h - 14},
                                                this->rect().w - 8, 12});
        _decErrorView->isVisible(true);
    } else if (_decErrorView->isVisible()) {
        _decErrorView->isVisible(false);
    }
}

KeyHandlingReaction InspectScreen::_handleKey(const int key)
{
    if (_decErrorView->isVisible()) {
        _decErrorView->isVisible(false);
        _ertView->redraw();
    }

    switch (key) {
    case 't':
        _tsFormatModeWheel.next();
        _ertView->timestampFormatMode(_tsFormatModeWheel.currentValue());
        break;

    case 's':
        _dsFormatModeWheel.next();
        _ertView->dataSizeFormatMode(_dsFormatModeWheel.currentValue());
        break;

    case 'c':
        this->_state().activeDataStreamFileState().gotoPacketContext();
        break;

    case KEY_HOME:
        this->_state().activeDataStreamFileState().curOffsetInPacketBits(0);
        break;

    case KEY_END:
        this->_state().activeDataStreamFileState().gotoLastDataRegion();
        break;

    case KEY_LEFT:
        this->_state().activeDataStreamFileState().gotoPreviousDataRegion();
        break;

    case KEY_RIGHT:
        this->_state().activeDataStreamFileState().gotoNextDataRegion();
        break;

#if 0
    case '/':
    case 'g':
    {
        _searchView->isVisible(true);

        auto input = _searchView->input();

        _searchView->isVisible(false);
        _ertView->redraw();
        break;
    }
#endif

    case '-':
        this->_state().activeDataStreamFileState().gotoPreviousEventRecord();
        break;

    case '+':
    case '=':
    case ' ':
        this->_state().activeDataStreamFileState().gotoNextEventRecord();
        break;

    case KEY_F(3):
        this->_state().gotoPreviousDataStreamFile();
        this->_tryShowDecodingError();
        break;

    case KEY_F(4):
        this->_state().gotoNextDataStreamFile();
        this->_tryShowDecodingError();
        break;

    case KEY_F(5):
        this->_state().gotoPreviousPacket();
        this->_tryShowDecodingError();
        break;

    case KEY_F(6):
        this->_state().gotoNextPacket();
        this->_tryShowDecodingError();
        break;

    case KEY_F(7):
        this->_state().activeDataStreamFileState().gotoPreviousEventRecord(10);
        break;

    case KEY_F(8):
        this->_state().activeDataStreamFileState().gotoNextEventRecord(10);
        break;

    default:
        break;
    }

    _ertView->refresh();
    _dtPathView->refresh();

    /*
     * Touch because the content could be unchanged from the last
     * refresh, and since this is overlapping other views, and they were
     * just refreshed, ncurses's optimization could ignore this refresh
     * otherwise.
     */
    _decErrorView->refresh(true);
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
