/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <utility>

#include "../key.hpp"
#include "dts-screen.hpp"
#include "data/content-pkt-region.hpp"
#include "inspect-common/search-query.hpp"

namespace jacques {

DtsScreen::DtsScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                     ScreenHost& host) :
    Screen {rect, stylist, appState, host},
    _searchController {*this, stylist, host}
{
    const auto viewRects = this->_viewRects();

    _dstTableView = std::make_unique<DstTableView>(std::get<0>(viewRects), stylist, appState);
    _ertTableView = std::make_unique<ErtTableView>(std::get<1>(viewRects), stylist, appState);
    _dtExplorerView = std::make_unique<DtExplorerView>(std::get<2>(viewRects), stylist);
    this->_addView(*_dstTableView);
    this->_addView(*_ertTableView);
    this->_addView(*_dtExplorerView);
    this->_focus(*_dstTableView);
    this->_updateViews();
}

void DtsScreen::_visibilityChanged()
{
    if (this->isVisible()) {
        // the active trace could have changed in the meantime
        this->_updateViews();
    }
}

std::tuple<Rect, Rect, Rect> DtsScreen::_viewRects() const
{
    const auto& rect = this->rect();
    const auto detailsWidth = _tablesVisible ? rect.w - (rect.w / 2) : rect.w;
    const auto detailsX = _tablesVisible ? rect.w / 2 : 0;
    const Rect dstTableViewRect {rect.pos, rect.w / 2, 6};
    const Rect ertTableViewRect {{rect.pos.x, rect.pos.y + dstTableViewRect.h},
                                 rect.w / 2, rect.h - dstTableViewRect.h};
    const Rect dtExplorerViewRect {{detailsX, rect.pos.y}, detailsWidth, rect.h};

    return std::tuple {dstTableViewRect, ertTableViewRect, dtExplorerViewRect};
}

void DtsScreen::_updateViews()
{
    _dstTableView->traceType(this->_appState().metadata().traceType());
    _ertTableView->dst(*_dstTableView->dst());
    _dstTableView->highlightSel(false);
    _ertTableView->highlightSel(false);

    if (this->_focusedView() == _dstTableView.get()) {
        _dtExplorerView->dst(*_dstTableView->dst());
        _dstTableView->highlightSel(true);
    } else if (this->_focusedView() == _ertTableView.get()) {
        _dtExplorerView->ert(*_ertTableView->ert());
        _ertTableView->highlightSel(true);
    }
}

void DtsScreen::_resized()
{
    const auto viewRects = this->_viewRects();

    _dstTableView->moveAndResize(std::get<0>(viewRects));
    _ertTableView->moveAndResize(std::get<1>(viewRects));
    _dtExplorerView->moveAndResize(std::get<2>(viewRects));
    _searchController.parentScreenResized(*this);
}

void DtsScreen::highlightCurDt()
{
    if (!this->_appState().hasActivePktState()) {
        _dtExplorerView->clearHighlight();
        return;
    }

    auto& activePktState = this->_appState().activePktState();

    const auto curDst = activePktState.pktIndexEntry().dst();
    const auto curEr = this->_appState().curEr();

    if (curEr && curEr->type()) {
        assert(curDst);

        _dstTableView->selectDst(curDst->id());
        _ertTableView->dst(*curDst);
        _ertTableView->selectErt(curEr->type()->id());
        _dstTableView->centerSelRow();
        _ertTableView->centerSelRow();
        _dtExplorerView->ert(*curEr->type());
        this->_focus(*_ertTableView);
    } else if (curDst) {
        _dstTableView->selectDst(curDst->id());
        _ertTableView->dst(*curDst);
        _ertTableView->selectFirst();
        _dstTableView->centerSelRow();
        _ertTableView->centerSelRow();
        _dtExplorerView->dst(*curDst);
        this->_focus(*_dstTableView);
    }

    _dtExplorerView->clearHighlight();

    const auto curPktRegion = this->_appState().curPktRegion();

    if (curPktRegion) {
        const auto cPktRegion = dynamic_cast<const ContentPktRegion *>(curPktRegion);

        if (cPktRegion) {
            _dtExplorerView->highlightDt(cPktRegion->dt());
            this->_focus(*_dtExplorerView);
        }
    }
}

void DtsScreen::clearHighlight()
{
    _dtExplorerView->clearHighlight();
}

void DtsScreen::_focusedViewHandledKey(Key)
{
    // the selection of a table view could have changed
    this->_updateViews();
}

void DtsScreen::_searchAccepted(std::unique_ptr<const SearchQuery> query)
{
    if (!query) {
        // canceled or invalid
        return;
    }

    _lastQuery = nullptr;

    if (const auto sQuery = dynamic_cast<const ErtNameSearchQuery *>(query.get())) {
        _ertTableView->selectErt(sQuery->pattern());
        _lastQuery.reset(sQuery);
        query.release();
    } else if (const auto sQuery = dynamic_cast<const ErtIdSearchQuery *>(query.get())) {
        _ertTableView->selectErt(static_cast<yactfr::TypeId>(sQuery->val()));
    }

    this->_updateViews();
    _ertTableView->centerSelRow();
}

void DtsScreen::_handleKey(const Key key)
{
    const auto focusedView = this->_focusedView();

    if (const auto specialKey = key.special()) {
        switch (*specialKey) {
        case SpecialKey::ENTER:
        case SpecialKey::TAB:
            if (!_tablesVisible) {
                break;
            }

            // change focus: data stream types, event record types, data types
            if (focusedView == _ertTableView.get()) {
                this->_focus(*_dtExplorerView);
            } else if (focusedView == _dtExplorerView.get()) {
                this->_focus(*_dstTableView);
            } else if (_ertTableView->isEmpty()) {
                this->_focus(*_dtExplorerView);
            } else {
                this->_focus(*_ertTableView);
            }

            this->_updateViews();
            break;

        case SpecialKey::LEFT:
            if (!_tablesVisible) {
                break;
            }

            if (_ertTableView->isEmpty()) {
                this->_focus(*_dstTableView);
            } else {
                this->_focus(*_ertTableView);
            }

            this->_updateViews();
            break;

        case SpecialKey::RIGHT:
            if (!_tablesVisible || focusedView == _dtExplorerView.get()) {
                break;
            }

            this->_focus(*_dtExplorerView);
            this->_updateViews();
            break;

        default:
            break;
        }
    } else {
        const auto ch = *key.ch();

        switch (ch) {
        case '+':
        {
            _tablesVisible = !_tablesVisible;

            const auto viewRects = this->_viewRects();

            _dtExplorerView->moveAndResize(std::get<2>(viewRects));
            _dstTableView->isVisible(_tablesVisible);
            _ertTableView->isVisible(_tablesVisible);

            if (!_tablesVisible) {
                this->_focus(*_dtExplorerView);
            }

            break;
        }

        case '/':
        case 'g':
        {
            if (!_tablesVisible || focusedView != _ertTableView.get()) {
                break;
            }

            _searchController.start([this](std::unique_ptr<const SearchQuery> query) {
                this->_searchAccepted(std::move(query));
            });

            break;
        }

        case 'n':
            if (!_tablesVisible || focusedView != _ertTableView.get() || !_lastQuery) {
                break;
            }

            _ertTableView->selectErt(_lastQuery->pattern(), true);
            this->_updateViews();
            _ertTableView->centerSelRow();
            break;

        default:
            break;
        }
    }
}

} // namespace jacques
