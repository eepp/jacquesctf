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
#include "../views/packet-table-view.hpp"
#include "../views/search-input-view.hpp"
#include "packets-screen.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "data/packet-checkpoints-build-listener.hpp"
#include "../views/packet-checkpoints-build-progress-view.hpp"

namespace jacques {

PacketsScreen::PacketsScreen(const Rectangle& rect, const InspectConfig& cfg,
                             const Stylist& stylist, State& state) :
    Screen {rect, cfg, stylist, state},
    _ptView {std::make_unique<PacketTableView>(rect, stylist, state)},
    _searchController {*this, stylist},
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
    _ptView->focus();
}

void PacketsScreen::_redraw()
{
    _ptView->redraw();
}

void PacketsScreen::_visibilityChanged()
{
    _ptView->isVisible(this->isVisible());

    if (this->isVisible()) {
        _ptView->redraw();
    }
}

void PacketsScreen::_resized()
{
    _ptView->moveAndResize(this->rect());
    _searchController.parentScreenResized(*this);
}

class AnalyzeAllPacketsProgressUpdater :
    public PacketCheckpointsBuildListener
{
public:
    explicit AnalyzeAllPacketsProgressUpdater(const Stylist& stylist) :
        _view {
            std::make_unique<PacketCheckpointsBuildProgressView>(
                Rectangle {{4, 4}, static_cast<Size>(COLS) - 8, 12},
                stylist
            )
        }
    {
        _view->focus();
        _view->isVisible(true);
    }

private:
    void _startBuild(const PacketIndexEntry& packetIndexEntry) override
    {
        ++_pktCount;

        if (packetIndexEntry.effectiveTotalSize() >= 2_MiB ||
                _pktCount % 49 == 1) {
            _canUpdate = true;
        } else {
            _canUpdate = false;
            return;
        }

        _view->packetIndexEntry(packetIndexEntry);
        _view->refresh(true);
        doupdate();
        _count = 0;
    }

    void _update(const EventRecord& eventRecord) override
    {
        if (!_canUpdate) {
            return;
        }

        if (_count++ % 41 != 0) {
            return;
        }

        _view->eventRecord(eventRecord);
        _view->refresh();
        doupdate();
    }

private:
    bool _canUpdate = false;
    Index _pktCount = 0;
    Index _count = 0;
    std::unique_ptr<PacketCheckpointsBuildProgressView> _view;
};

KeyHandlingReaction PacketsScreen::_handleKey(const int key)
{
    switch (key) {
    case KEY_UP:
        _ptView->prev();
        break;

    case KEY_DOWN:
        _ptView->next();
        break;

    case KEY_PPAGE:
        _ptView->pageUp();
        break;

    case KEY_NPAGE:
        _ptView->pageDown();
        break;

    case KEY_END:
        _ptView->selectLast();
        break;

    case KEY_HOME:
        _ptView->selectFirst();
        break;

    case 'c':
        _ptView->centerSelectedRow();
        break;

    case 't':
        _tsFormatModeWheel.next();
        _ptView->timestampFormatMode(_tsFormatModeWheel.currentValue());
        break;

    case 's':
        _dsFormatModeWheel.next();
        _ptView->dataSizeFormatMode(_dsFormatModeWheel.currentValue());
        break;

    case '/':
    case 'g':
    case 'N':
    case 'k':
    case ':':
    case '$':
    case '*':
    case 'P':
    {
        std::string init;

        switch (key) {
        case 'N':
        case '*':
            init = "*";
            break;

        case 'k':
            init = "**";
            break;

        case ':':
            init = ":";
            break;

        case 'P':
            init = "#";
            break;

        case '$':
            init = "$";
            break;

        default:
            break;
        }

        auto query = _searchController.start(init);

        if (!query) {
            // canceled or invalid
            this->_redraw();
            break;
        }

        this->_state().search(*query);

        _lastQuery = std::move(query);
        this->_redraw();
        break;
    }

    case 'n':
        if (!_lastQuery) {
            break;
        }

        this->_state().search(*_lastQuery);
        _ptView->redraw();
        break;

    case '\n':
    case '\r':
        if (this->_state().activeDataStreamFileState().dataStreamFile().packetCount() == 0) {
            break;
        }

        this->_state().gotoPacket(_ptView->selectedPacketIndex());
        return KeyHandlingReaction::RETURN_TO_INSPECT;

    case KEY_F(3):
        this->_state().gotoPreviousDataStreamFile();
        break;

    case KEY_F(4):
        this->_state().gotoNextDataStreamFile();
        break;

    case KEY_F(5):
        this->_state().gotoPreviousPacket();
        break;

    case KEY_F(6):
        this->_state().gotoNextPacket();
        break;

    case 'a':
    {
        auto updater = AnalyzeAllPacketsProgressUpdater {this->_stylist()};

        this->_state().activeDataStreamFileState().analyzeAllPackets(updater);
        _ptView->redraw();
        break;
    }

    default:
        break;
    }

    _ptView->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
