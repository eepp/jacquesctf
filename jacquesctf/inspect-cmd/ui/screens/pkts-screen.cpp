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
#include "../views/pkt-table-view.hpp"
#include "../views/search-input-view.hpp"
#include "pkts-screen.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "data/pkt-checkpoints-build-listener.hpp"
#include "../views/pkt-checkpoints-build-progress-view.hpp"

namespace jacques {

PktsScreen::PktsScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                       State& state) :
    Screen {rect, cfg, stylist, state},
    _ptView {std::make_unique<PktTableView>(rect, stylist, state)},
    _searchCtrl {*this, stylist},
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
    _ptView->focus();
}

void PktsScreen::_redraw()
{
    _ptView->redraw();
}

void PktsScreen::_visibilityChanged()
{
    _ptView->isVisible(this->isVisible());

    if (this->isVisible()) {
        _ptView->redraw();
    }
}

void PktsScreen::_resized()
{
    _ptView->moveAndResize(this->rect());
    _searchCtrl.parentScreenResized(*this);
}

namespace {

class AnalyzeAllPktsProgressUpdater final :
    public PktCheckpointsBuildListener
{
public:
    explicit AnalyzeAllPktsProgressUpdater(const Stylist& stylist) :
        _view {
            std::make_unique<PktCheckpointsBuildProgressView>(
                Rect {{4, 4}, static_cast<Size>(COLS) - 8, 12},
                stylist
            )
        }
    {
        _view->focus();
        _view->isVisible(true);
    }

private:
    void _startBuild(const PktIndexEntry& pktIndexEntry) override
    {
        ++_pktCount;

        if (pktIndexEntry.effectiveTotalLen() >= 2_MiB || _pktCount % 49 == 1) {
            _canUpdate = true;
        } else {
            _canUpdate = false;
            return;
        }

        _view->pktIndexEntry(pktIndexEntry);
        _view->refresh(true);
        doupdate();
        _count = 0;
    }

    void _update(const Er& er) override
    {
        if (!_canUpdate) {
            return;
        }

        if (_count++ % 41 != 0) {
            return;
        }

        _view->er(er);
        _view->refresh();
        doupdate();
    }

private:
    bool _canUpdate = false;
    Index _pktCount = 0;
    Index _count = 0;
    std::unique_ptr<PktCheckpointsBuildProgressView> _view;
};

} // namespace

KeyHandlingReaction PktsScreen::_handleKey(const int key)
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
        _ptView->centerSelRow();
        break;

    case 't':
        _tsFmtModeWheel.next();
        _ptView->tsFmtMode(_tsFmtModeWheel.curVal());
        break;

    case 's':
        _dataLenFmtModeWheel.next();
        _ptView->dataLenFmtMode(_dataLenFmtModeWheel.curVal());
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
        auto query = _searchCtrl.start([key]() -> std::string {
            switch (key) {
            case 'N':
            case '*':
                return "*";
                break;

            case 'k':
                return "**";
                break;

            case ':':
                return ":";
                break;

            case 'P':
                return "#";
                break;

            case '$':
                return "$";
                break;

            default:
                return {};
            }
        }());

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
        if (this->_state().activeDsFileState().dsFile().pktCount() == 0) {
            break;
        }

        this->_state().gotoPkt(_ptView->selPktIndex());
        return KeyHandlingReaction::RETURN_TO_INSPECT;

    case KEY_F(3):
        this->_state().gotoPrevDsFile();
        break;

    case KEY_F(4):
        this->_state().gotoNextDsFile();
        break;

    case KEY_F(5):
        this->_state().gotoPrevPkt();
        break;

    case KEY_F(6):
        this->_state().gotoNextPkt();
        break;

    case 'a':
    {
        AnalyzeAllPktsProgressUpdater updater {this->_stylist()};

        this->_state().activeDsFileState().analyzeAllPkts(updater);
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
