/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <iostream>
#include <thread>
#include <atomic>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

#include "cfg.hpp"
#include "../views/er-table-view.hpp"
#include "inspect-screen.hpp"
#include "../stylist.hpp"
#include "../views/pkt-data-view.hpp"
#include "utils.hpp"

namespace jacques {

InspectScreen::InspectScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                             InspectCmdState& appState) :
    Screen {rect, cfg, stylist, appState},
    _decErrorView {
        std::make_unique<PktDecodingErrorDetailsView>(rect, stylist, appState)
    },
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
    },
    _curStateSnapshot {_stateSnapshots.end()},
    _ertViewDispModeWheel {
        _ErtViewDispMode::SHORT,
        _ErtViewDispMode::LONG,
        _ErtViewDispMode::FULL,
        _ErtViewDispMode::HIDDEN,
    }
{
    _pdView = std::make_unique<PktDataView>(this->rect(), stylist, appState, _bookmarks);
    _ertView = std::make_unique<ErTableView>(this->rect(), stylist, appState);
    _priView = std::make_unique<PktRegionInfoView>(Rect {{0, 0}, this->rect().w, 1}, stylist,
                                                   appState);
    _sdteView = std::make_unique<SubDtExplorerView>(this->rect(), stylist, appState);
    _decErrorView->isVisible(false);
    _pdView->focus();
    this->_updateViews();
}

InspectScreen::~InspectScreen()
{
}

InspectScreen::PktBookmarks& InspectScreen::_curPktBookmarks() noexcept
{
    const auto dsFileStateIndex = this->_appState().activeDsFileStateIndex();
    const auto pktStateIndex = this->_appState().activeDsFileState().activePktStateIndex();

    return _bookmarks[dsFileStateIndex][pktStateIndex];
}

void InspectScreen::_toggleBookmark(const unsigned int id)
{
    if (!this->_appState().hasActivePktState()) {
        return;
    }

    auto& bookmarks = this->_curPktBookmarks();

    assert(id < bookmarks.size());

    auto& bookmarkedOffsetInPktBit = bookmarks[id];

    if (bookmarkedOffsetInPktBit) {
        if (bookmarkedOffsetInPktBit == this->_appState().curOffsetInPktBits()) {
            bookmarkedOffsetInPktBit = boost::none;
        } else {
            bookmarkedOffsetInPktBit = this->_appState().curOffsetInPktBits();
        }
    } else {
        bookmarkedOffsetInPktBit = this->_appState().curOffsetInPktBits();
    }

    _pdView->redraw();
}

void InspectScreen::_gotoBookmark(const unsigned int id)
{
    if (!this->_appState().hasActivePktState()) {
        return;
    }

    const auto& bookmark = this->_curPktBookmarks()[id];

    if (bookmark) {
        this->_appState().gotoPktRegionAtOffsetInPktBits(*bookmark);
    }
}

void InspectScreen::_updateViews()
{
    Size ertViewHeight = 0;

    switch (_ertViewDispModeWheel.curVal()) {
    case _ErtViewDispMode::SHORT:
        ertViewHeight = 8;
        break;

    case _ErtViewDispMode::LONG:
        ertViewHeight = this->rect().h / 2;
        break;

    case _ErtViewDispMode::FULL:
        if (this->isVisible()) {
            _pdView->isVisible(false);
            _sdteView->isVisible(false);
            _priView->isVisible(false);
            _ertView->isVisible(true);
        }

        _ertView->moveAndResize(this->rect());
        _ertView->centerSelRow(false);
        return;

    default:
        break;
    }

    const auto pdViewHeight = this->rect().h - ertViewHeight - 1;
    const auto pdViewWidth = _sdteViewIsVisible ? this->rect().w / 2 : this->rect().w;

    if (_ertViewDispModeWheel.curVal() == _ErtViewDispMode::HIDDEN) {
        _ertView->moveAndResize({{0, 0}, this->rect().w, 8});

        if (this->isVisible()) {
            _ertView->isVisible(false);
        }
    } else {
        _ertView->moveAndResize({{0, pdViewHeight + 1}, this->rect().w, ertViewHeight});

        if (this->isVisible()) {
            _ertView->isVisible(true);
        }
    }

    if (this->isVisible()) {
        _pdView->isVisible(true);
        _priView->isVisible(true);
    }

    _pdView->moveAndResize({{0, 0}, pdViewWidth, pdViewHeight});
    _priView->moveAndResize({{0, pdViewHeight}, this->rect().w, 1});

    if (_sdteViewIsVisible) {
        _sdteView->moveAndResize({{pdViewWidth, 0}, this->rect().w - pdViewWidth, pdViewHeight});
    } else {
        _sdteView->moveAndResize({{0, 0}, this->rect().w, pdViewHeight});
    }

    if (this->isVisible()) {
        _sdteView->isVisible(_sdteViewIsVisible);
    }

    _decErrorView->moveAndResize({
        {this->rect().pos.x + 4, this->rect().h - 14},
        this->rect().w - 8, 12
    });
    _ertView->centerSelRow(false);

    // override visibility if screen is not visible
    if (!this->isVisible()) {
        _pdView->isVisible(false);
        _ertView->isVisible(false);
        _priView->isVisible(false);
        _sdteView->isVisible(false);
        _decErrorView->isVisible(false);
    }
}

void InspectScreen::_redraw()
{
    _pdView->redraw();
    _ertView->redraw();
    _priView->redraw();
    _sdteView->redraw();
    _decErrorView->redraw();
}

void InspectScreen::_resized()
{
    this->_updateViews();
    _searchCtrl.parentScreenResized(*this);
}

void InspectScreen::_visibilityChanged()
{
    _pdView->isVisible(this->isVisible());
    _priView->isVisible(this->isVisible());
    this->_updateViews();

    if (this->isVisible()) {
        if (_stateSnapshots.empty()) {
            // initial snapshot
            this->_snapshotState();
        }

        this->_redraw();
        this->_tryShowDecodingError();
        _decErrorView->refresh(true);
    }
}

void InspectScreen::_tryShowDecodingError()
{
    if (this->_appState().hasActivePktState() &&
            this->_appState().activePktState().pkt().error()) {
        _decErrorView->moveAndResize({
            {this->rect().pos.x + 4, this->rect().h - 14},
            this->rect().w - 8, 12
        });
        _decErrorView->isVisible(true);
    } else if (_decErrorView->isVisible()) {
        _decErrorView->isVisible(false);
    }
}

InspectScreen::_StateSnapshot InspectScreen::_takeStateSnapshot()
{
    _StateSnapshot snapshot;

    snapshot.dsfStateIndex = this->_appState().activeDsFileStateIndex();

    if (this->_appState().hasActivePktState()) {
        const auto& activeDsfState = this->_appState().activeDsFileState();

        snapshot.pktIndexInDsFile = activeDsfState.activePktStateIndex();
        snapshot.offsetInPktBits = activeDsfState.curOffsetInPktBits();
    }

    return snapshot;
}

void InspectScreen::_snapshotState()
{
    const auto snapshot = this->_takeStateSnapshot();

    if (!_stateSnapshots.empty()) {
        if (*_curStateSnapshot == snapshot) {
            // unchanged
            return;
        }

        const auto nextSnapshot = std::next(_curStateSnapshot);

        if (nextSnapshot != _stateSnapshots.end()) {
            _stateSnapshots.erase(nextSnapshot, _stateSnapshots.end());
        }
    }

    _stateSnapshots.push_back(snapshot);
    _curStateSnapshot = std::prev(_stateSnapshots.end());

    if (_stateSnapshots.size() > _maxStateSnapshots) {
        _stateSnapshots.pop_front();
        assert(_stateSnapshots.size() == _maxStateSnapshots);
    }
}

void InspectScreen::_goBack()
{
    if (_stateSnapshots.empty()) {
        return;
    }

    if (_curStateSnapshot == _stateSnapshots.begin()) {
        // can't go back
        return;
    }

    --_curStateSnapshot;
    this->_restoreStateSnapshot(*_curStateSnapshot);
}

void InspectScreen::_goForward()
{
    if (_stateSnapshots.empty()) {
        return;
    }

    if (std::next(_curStateSnapshot) == _stateSnapshots.end()) {
        // can't go forward
        return;
    }

    ++_curStateSnapshot;
    this->_restoreStateSnapshot(*_curStateSnapshot);
}

void InspectScreen::_restoreStateSnapshot(const _StateSnapshot& snapshot)
{
    this->_appState().gotoDsFile(snapshot.dsfStateIndex);

    if (snapshot.pktIndexInDsFile) {
        this->_appState().gotoPkt(*snapshot.pktIndexInDsFile);
        assert(this->_appState().hasActivePktState());
        this->_appState().gotoPktRegionAtOffsetInPktBits(snapshot.offsetInPktBits);
    }
}

void InspectScreen::_refreshViews()
{
    _pdView->refresh();
    _ertView->refresh();
    _priView->refresh();
    _sdteView->refresh();
}

void InspectScreen::_setLastOffsetInRowBits()
{
    if (!_lastOffsetInRowBits) {
        _lastOffsetInRowBits = this->_appState().curOffsetInPktBits() % _pdView->rowSize().bits();
    }
}

void InspectScreen::_search(const SearchQuery& query, const bool animate)
{
    if (dynamic_cast<const ErtNameSearchQuery *>(&query) ||
            dynamic_cast<const ErtIdSearchQuery *>(&query)) {
        std::atomic_bool stop {false};
        std::thread t {[this, &stop, &query] {
            this->_appState().search(query);
            stop = true;
        }};

        if (animate) {
            _searchCtrl.animate(stop);
        }

        t.join();
    } else {
        this->_appState().search(query);
    }
}

KeyHandlingReaction InspectScreen::_handleKey(const int key)
{
    const auto goingToBookmark = _goingToBookmark;
    const auto decErrorViewWasVisible = _decErrorView->isVisible();

    _goingToBookmark = false;

    if (_decErrorView->isVisible()) {
        _decErrorView->isVisible(false);
        this->_redraw();
    }

    if (key != KEY_DOWN && key != KEY_UP) {
        _lastOffsetInRowBits = boost::none;
    }

    switch (key) {
    case 127:
    case 8:
    case '9':
    case KEY_BACKSPACE:
        this->_goBack();
        break;

    case '0':
        this->_goForward();
        break;

    case '#':
    case '`':
        this->_tryShowDecodingError();
        break;

    case 't':
        _tsFmtModeWheel.next();
        _ertView->tsFmtMode(_tsFmtModeWheel.curVal());
        break;

    case 's':
        _dataLenFmtModeWheel.next();
        _ertView->dataLenFmtMode(_dataLenFmtModeWheel.curVal());
        break;

    case 'e':
        _ertViewDispModeWheel.next();
        this->_updateViews();
        this->_redraw();
        break;

    case 'E':
        _pdView->isErFirstPktRegionEmphasized(!_pdView->isErFirstPktRegionEmphasized());
        _pdView->redraw();
        break;

    case 'a':
        _pdView->isAsciiVisible(!_pdView->isAsciiVisible());
        break;

    case 'v':
        _pdView->isPrevNextVisible(!_pdView->isPrevNextVisible());
        break;

    case 'w':
        _pdView->isRowSizePowerOfTwo(!_pdView->isRowSizePowerOfTwo());
        break;

    case 'x':
        _pdView->isDataInHex(!_pdView->isDataInHex());
        break;

    case 'D':
        _pdView->isOffsetInPkt(!_pdView->isOffsetInPkt());
        break;

    case 'o':
        _pdView->isOffsetInHex(!_pdView->isOffsetInHex());
        break;

    case 'O':
        _pdView->isOffsetInBytes(!_pdView->isOffsetInBytes());
        break;

    case 'c':
        _pdView->centerSel();
        break;

    case '\n':
    case '\r':
        if (decErrorViewWasVisible) {
            // use Enter to close the box in this case
            break;
        }

        _sdteViewIsVisible = !_sdteViewIsVisible;
        this->_updateViews();
        this->_redraw();
        break;

    case 'C':
        this->_appState().gotoPktCtx();
        this->_snapshotState();
        break;

    case 'z':
        if (!this->_appState().hasActivePktState()) {
            break;
        }

        this->_appState().activePktState().gotoPktRegionNextParent();
        this->_snapshotState();
        break;

    case '1':
    case '2':
    case '3':
    case '4':
        if (goingToBookmark) {
            this->_gotoBookmark(key - '1');
        } else {
            this->_toggleBookmark(key - '1');
        }

        break;

    case 'b':
        _goingToBookmark = true;
        break;

    case KEY_HOME:
        this->_appState().gotoPktRegionAtOffsetInPktBits(0);
        this->_snapshotState();
        break;

    case KEY_END:
        this->_appState().gotoLastPktRegion();
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case KEY_LEFT:
        this->_appState().gotoPrevPktRegion();
        this->_snapshotState();
        break;

    case KEY_RIGHT:
        this->_appState().gotoNextPktRegion();
        this->_snapshotState();
        break;

    case KEY_UP:
    {
        if (!this->_appState().hasActivePktState()) {
            break;
        }

        auto& pkt = this->_appState().activePktState().pkt();

        if (!this->_appState().activePktState().pkt().hasData()) {
            break;
        }

        Index reqOffsetInPktBits;

        if (this->_appState().curOffsetInPktBits() < _pdView->rowSize()) {
            break;
        } else {
            const auto curOffsetInPktBits = this->_appState().curOffsetInPktBits();
            const auto rowSizeBits = _pdView->rowSize().bits();

            this->_setLastOffsetInRowBits();

            const auto rowOffsetInPktBits = curOffsetInPktBits - (curOffsetInPktBits % rowSizeBits);

            reqOffsetInPktBits = rowOffsetInPktBits - rowSizeBits + *_lastOffsetInRowBits;
        }

        const auto& reqPktRegion = pkt.regionAtOffsetInPktBits(reqOffsetInPktBits);

        this->_appState().gotoPktRegionAtOffsetInPktBits(reqPktRegion.segment().offsetInPktBits());
        this->_snapshotState();
        break;
    }

    case KEY_DOWN:
    {
        if (!this->_appState().hasActivePktState()) {
            break;
        }

        auto& pkt = this->_appState().activePktState().pkt();

        if (!pkt.hasData()) {
            break;
        }

        this->_setLastOffsetInRowBits();

        const auto& curPktRegion = *this->_appState().curPktRegion();
        const auto nextOffsetInPktBits = this->_appState().curOffsetInPktBits() -
                                         (this->_appState().curOffsetInPktBits() %
                                          _pdView->rowSize().bits()) +
                                         _pdView->rowSize().bits() + *_lastOffsetInRowBits;
        const auto reqOffsetInPktBits = std::max(nextOffsetInPktBits,
                                                 *curPktRegion.segment().endOffsetInPktBits());

        if (reqOffsetInPktBits > pkt.lastRegion().segment().offsetInPktBits()) {
            break;
        }

        const auto& reqPktRegion = pkt.regionAtOffsetInPktBits(reqOffsetInPktBits);

        this->_appState().gotoPktRegionAtOffsetInPktBits(reqPktRegion.segment().offsetInPktBits());
        this->_snapshotState();
        break;
    }

    case KEY_PPAGE:
        _pdView->pageUp();
        break;

    case KEY_NPAGE:
        _pdView->pageDown();
        break;

    case '/':
    case 'g':
    case 'G':
    case 'N':
    case 'k':
    case ':':
    case '$':
    case '*':
    case 'P':
    {
        const auto init = utils::call([key]() {
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
                return "";
            }
        });

        std::unique_ptr<const SearchQuery> query;

        if (key == 'G') {
            const auto snapshot = this->_takeStateSnapshot();
            const auto liveUpdateFunc = [this, snapshot](const auto& query) {
                // start from initial state
                this->_restoreStateSnapshot(snapshot);

                if (!dynamic_cast<const ErtNameSearchQuery *>(&query) &&
                        !dynamic_cast<const ErtIdSearchQuery *>(&query)) {
                    // this makes the appropriate views update and redraw
                    this->_appState().search(query);
                }

                // refresh background views
                this->_refreshViews();
            };

            query = _searchCtrl.startLive(init, liveUpdateFunc);

            if (key == 'G') {
                this->_restoreStateSnapshot(snapshot);
            }
        } else {
            query = _searchCtrl.start(init);
        }

        if (!query) {
            // canceled or invalid
            this->_redraw();
            break;
        }

        this->_search(*query);

        /*
         * If we didn't move, the state snapshot will be identical and
         * will be not be pushed to the state snapshot list.
         */
        this->_snapshotState();

        _lastQuery = std::move(query);
        this->_redraw();
        this->_tryShowDecodingError();
        break;
    }

    case 'n':
        if (!_lastQuery) {
            break;
        }

        this->_search(*_lastQuery, false);
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case '-':
        this->_appState().gotoPrevEr();
        this->_snapshotState();
        break;

    case '+':
    case '=':
    case ' ':
        this->_appState().gotoNextEr();
        this->_snapshotState();
        break;

    case KEY_F(3):
        this->_appState().gotoPrevDsFile();
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case KEY_F(4):
        this->_appState().gotoNextDsFile();
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case KEY_F(5):
        this->_appState().gotoPrevPkt();
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case KEY_F(6):
        this->_appState().gotoNextPkt();
        this->_snapshotState();
        this->_tryShowDecodingError();
        break;

    case KEY_F(7):
        this->_appState().gotoPrevEr(10);
        this->_snapshotState();
        break;

    case KEY_F(8):
        this->_appState().gotoNextEr(10);
        this->_snapshotState();
        break;

    default:
        break;
    }

    this->_refreshViews();

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
