/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <functional>
#include <initializer_list>
#include <utility>

#include "../key.hpp"
#include "../views/er-table-view.hpp"
#include "inspect-screen.hpp"
#include "../stylist.hpp"
#include "../views/pkt-data-view.hpp"

namespace jacques {

InspectScreen::InspectScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                             ScreenHost& host) :
    Screen {rect, stylist, appState, host},
    _decErrorView {
        std::make_unique<PktDecodingErrorDetailsView>(this->_decErrorViewRect(), stylist,
                                                      appState)
    },
    _searchCtrl {*this, stylist, host},
    _curStateSnapshot {_stateSnapshots.end()},
    _ertViewDispModeWheel {
        _ErtViewDispMode::SHORT,
        _ErtViewDispMode::LONG,
        _ErtViewDispMode::FULL,
        _ErtViewDispMode::HIDDEN,
    },
    _appStateObserverGuard {appState, [this](const Message msg) {
        this->_appStateChanged(msg);
    }}
{
    _pdView = std::make_unique<PktDataView>(this->rect(), stylist, appState, _bookmarks);
    _ertView = std::make_unique<ErTableView>(this->rect(), stylist, appState);
    _priView = std::make_unique<PktRegionInfoView>(Rect {{0, 0}, this->rect().w, 1}, stylist,
                                                   appState);
    _sdteView = std::make_unique<SubDtExplorerView>(this->rect(), stylist, appState);

    // in stacking order: overlays last
    for (const auto view : std::initializer_list<View *> {
        _pdView.get(), _ertView.get(), _priView.get(), _sdteView.get(), _decErrorView.get()
    }) {
        this->_addView(*view);
    }

    _decErrorView->isVisible(false);
    this->_focus(*_pdView);
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

    if (auto& bookmarkedOffsetInPktBit = bookmarks[id]) {
        if (bookmarkedOffsetInPktBit == this->_appState().curOffsetInPktBits()) {
            bookmarkedOffsetInPktBit = std::nullopt;
        } else {
            bookmarkedOffsetInPktBit = this->_appState().curOffsetInPktBits();
        }
    } else {
        bookmarkedOffsetInPktBit = this->_appState().curOffsetInPktBits();
    }

    _pdView->bookmarksChanged();
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
    const auto& rect = this->rect();
    const auto dispMode = _ertViewDispModeWheel.curVal();

    _decErrorView->moveAndResize(this->_decErrorViewRect());

    if (dispMode == _ErtViewDispMode::FULL) {
        _pdView->isVisible(false);
        _sdteView->isVisible(false);
        _priView->isVisible(false);
        _ertView->isVisible(true);
        _ertView->moveAndResize(rect);
        _ertView->centerSelRow();
        return;
    }

    const auto ertViewHeight = std::invoke([dispMode, &rect]() -> Size {
        switch (dispMode) {
        case _ErtViewDispMode::SHORT:
            return 8;

        case _ErtViewDispMode::LONG:
            return rect.h / 2;

        default:
            return 0;
        }
    });

    const auto pdViewHeight = rect.h - ertViewHeight - 1;
    const auto pdViewWidth = _sdteViewIsVisible ? rect.w / 2 : rect.w;

    _pdView->isVisible(true);
    _priView->isVisible(true);
    _sdteView->isVisible(_sdteViewIsVisible);
    _ertView->isVisible(dispMode != _ErtViewDispMode::HIDDEN);
    _pdView->moveAndResize({{0, 0}, pdViewWidth, pdViewHeight});
    _priView->moveAndResize({{0, pdViewHeight}, rect.w, 1});

    if (dispMode == _ErtViewDispMode::HIDDEN) {
        // keep a valid size while hidden
        _ertView->moveAndResize({{0, 0}, rect.w, 8});
    } else {
        _ertView->moveAndResize({{0, pdViewHeight + 1}, rect.w, ertViewHeight});
    }

    if (_sdteViewIsVisible) {
        _sdteView->moveAndResize({{pdViewWidth, 0}, rect.w - pdViewWidth, pdViewHeight});
    }

    _ertView->centerSelRow();
}

void InspectScreen::_resized()
{
    this->_updateViews();
    _searchCtrl.parentScreenResized(*this);
}

void InspectScreen::_visibilityChanged()
{
    if (!this->isVisible()) {
        return;
    }

    if (_stateSnapshots.empty()) {
        // initial snapshot
        this->_snapshotState();
    }

    this->_tryShowDecodingError();
}

void InspectScreen::_tryShowDecodingError()
{
    _decErrorView->isVisible(this->_appState().hasActivePktState() &&
                             this->_appState().activePktState().pkt().error());
}

void InspectScreen::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED || msg == Message::ACTIVE_PKT_CHANGED) {
        this->_tryShowDecodingError();
    }
}

Rect InspectScreen::_decErrorViewRect() const noexcept
{
    const auto& rect = this->rect();

    return {{rect.pos.x + 4, rect.h - 14}, rect.w - 8, 12};
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

void InspectScreen::_setLastOffsetInRowBits()
{
    if (!_lastOffsetInRowBits) {
        _lastOffsetInRowBits = this->_appState().curOffsetInPktBits() % _pdView->rowSize().bits();
    }
}

void InspectScreen::_search(const SearchQuery& query, const bool animate)
{
    const auto isSlow = dynamic_cast<const ErtNameSearchQuery *>(&query) ||
                        dynamic_cast<const ErtIdSearchQuery *>(&query);

    if (isSlow && animate) {
        _searchCtrl.animateWhile([this, &query](const auto& animateFunc) {
            this->_appState().search(query, animateFunc);
        });
    } else {
        this->_appState().search(query);
    }
}

void InspectScreen::_startSearch(const std::string& init, const bool live)
{
    if (!live) {
        _searchCtrl.start(init, [this](std::unique_ptr<const SearchQuery> query) {
            this->_searchAccepted(std::move(query));
        });

        return;
    }

    const auto snapshot = this->_takeStateSnapshot();
    const auto liveUpdateFunc = [this, snapshot](const auto& query) {
        // start from initial state
        this->_restoreStateSnapshot(snapshot);

        if (!dynamic_cast<const ErtNameSearchQuery *>(&query) &&
                !dynamic_cast<const ErtIdSearchQuery *>(&query)) {
            // this makes the appropriate views update and redraw
            this->_appState().search(query);
        }
    };

    _searchCtrl.startLive(init, liveUpdateFunc,
                          [this, snapshot](std::unique_ptr<const SearchQuery> query) {
        this->_restoreStateSnapshot(snapshot);
        this->_searchAccepted(std::move(query));
    });
}

void InspectScreen::_searchAccepted(std::unique_ptr<const SearchQuery> query)
{
    if (!query) {
        // canceled or invalid
        return;
    }

    this->_search(*query);

    /*
     * If we didn't move, then the state snapshot is identical and
     * _snapshotState() doesn't push it to the state snapshot list.
     */
    this->_snapshotState();

    _lastQuery = std::move(query);
}

void InspectScreen::_handleKey(const Key key)
{
    const auto goingToBookmark = _goingToBookmark;
    const auto decErrorViewWasVisible = _decErrorView->isVisible();

    /*
     * Any key hides the decoding error box, but _appStateChanged()
     * shows it again if the key changes the active packet to one having
     * a decoding error.
     */
    _goingToBookmark = false;
    _decErrorView->isVisible(false);

    if (key.special() != SpecialKey::DOWN && key.special() != SpecialKey::UP) {
        _lastOffsetInRowBits = std::nullopt;
    }

    if (this->_handleFmtModeKey(key)) {
        return;
    }

    // _searchAccepted() takes the state snapshot
    if (key.ch() == U'G') {
        this->_startSearch({}, true);
        return;
    } else if (const auto init = SearchCtrl::initFromKey(key)) {
        this->_startSearch(*init, false);
        return;
    }

    /*
     * Take a snapshot of the state after handling the key, unless the
     * key restores a previous state.
     *
     * _snapshotState() ignores an unchanged state.
     */
    auto snapshotState = true;

    if (this->_handleDsFileKey(key)) {
        // nothing else to do
    } else if (const auto specialKey = key.special()) {
        switch (*specialKey) {
        case SpecialKey::BACKSPACE:
            this->_goBack();
            snapshotState = false;
            break;

        case SpecialKey::ENTER:
            if (decErrorViewWasVisible) {
                // use Enter to close the box in this case
                break;
            }

            _sdteViewIsVisible = !_sdteViewIsVisible;
            this->_updateViews();
            break;

        case SpecialKey::HOME:
            this->_appState().gotoPktRegionAtOffsetInPktBits(0);
            break;

        case SpecialKey::END:
            this->_appState().gotoLastPktRegion();

            // the last region of a packet may be its decoding error
            this->_tryShowDecodingError();
            break;

        case SpecialKey::LEFT:
            this->_appState().gotoPrevPktRegion();
            break;

        case SpecialKey::RIGHT:
            this->_appState().gotoNextPktRegion();
            break;

        case SpecialKey::UP:
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

                const auto rowOffsetInPktBits = curOffsetInPktBits -
                                                (curOffsetInPktBits % rowSizeBits);

                reqOffsetInPktBits = rowOffsetInPktBits - rowSizeBits + *_lastOffsetInRowBits;
            }

            const auto& reqPktRegion = pkt.regionAtOffsetInPktBits(reqOffsetInPktBits);

            this->_appState().gotoPktRegionAtOffsetInPktBits(
                reqPktRegion.segment().offsetInPktBits()
            );
            break;
        }

        case SpecialKey::DOWN:
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

            this->_appState().gotoPktRegionAtOffsetInPktBits(
                reqPktRegion.segment().offsetInPktBits()
            );
            break;
        }

        case SpecialKey::PAGE_UP:
            _pdView->pageUp();
            break;

        case SpecialKey::PAGE_DOWN:
            _pdView->pageDown();
            break;

        case SpecialKey::F5:
            this->_appState().gotoPrevPkt();
            break;

        case SpecialKey::F6:
            this->_appState().gotoNextPkt();
            break;

        case SpecialKey::F7:
            this->_appState().gotoPrevEr(10);
            break;

        case SpecialKey::F8:
            this->_appState().gotoNextEr(10);
            break;

        default:
            break;
        }
    } else {
        const auto ch = *key.ch();

        switch (ch) {
        case '9':
            this->_goBack();
            snapshotState = false;
            break;

        case '0':
            this->_goForward();
            snapshotState = false;
            break;

        case '#':
        case '`':
            this->_tryShowDecodingError();
            break;

        case 'e':
            _ertViewDispModeWheel.next();
            this->_updateViews();
            break;

        case 'E':
            _pdView->isErFirstPktRegionEmphasized(!_pdView->isErFirstPktRegionEmphasized());
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

        case 'C':
            this->_appState().gotoPktCtx();
            break;

        case 'z':
            if (!this->_appState().hasActivePktState()) {
                break;
            }

            this->_appState().activePktState().gotoPktRegionNextParent();
            break;

        case '1':
        case '2':
        case '3':
        case '4':
            if (goingToBookmark) {
                this->_gotoBookmark(ch - '1');
                snapshotState = false;
            } else {
                this->_toggleBookmark(ch - '1');
            }

            break;

        case 'b':
            _goingToBookmark = true;
            break;

        case 'n':
            if (!_lastQuery) {
                break;
            }

            this->_search(*_lastQuery, false);
            break;

        case '-':
            this->_appState().gotoPrevEr();
            break;

        case '+':
        case '=':
        case ' ':
            this->_appState().gotoNextEr();
            break;

        default:
            break;
        }
    }

    if (snapshotState) {
        this->_snapshotState();
    }
}

} // namespace jacques
