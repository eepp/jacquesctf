/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <utility>

#include "dts-screen.hpp"
#include "data/content-pkt-region.hpp"
#include "inspect-common/search-query.hpp"

namespace jacques {

DtsScreen::DtsScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                     ScreenHost& host) :
    Screen {rect, stylist, appState, host},
    _searchCtrl {*this, stylist, host}
{
    const auto viewRects = this->_viewRects();

    _dstTableView = std::make_unique<DstTableView>(viewRects.dstTable, stylist, appState);
    _ertTableView = std::make_unique<ErtTableView>(viewRects.ertTable, stylist, appState);
    _dtExplorerView = std::make_unique<DtExplorerView>(viewRects.dtExplorer, stylist);
    this->_addView(*_dstTableView);
    this->_addView(*_ertTableView);
    this->_addView(*_dtExplorerView);
    this->_updateTraceType();
    this->_focusAndUpdateViews(*_dstTableView);
}

void DtsScreen::_visibilityChanged()
{
    if (this->isVisible()) {
        // the active trace could have changed in the meantime
        this->_updateTraceType();
        this->_updateViews();
    }
}

DtsScreen::_ViewRects DtsScreen::_viewRects() const
{
    const auto& rect = this->rect();
    const auto detailsWidth = _tablesVisible ? rect.w - (rect.w / 2) : rect.w;
    const auto detailsX = _tablesVisible ? rect.w / 2 : 0;
    const Rect dstTableViewRect {rect.pos, rect.w / 2, 6};
    const Rect ertTableViewRect {{rect.pos.x, rect.pos.y + dstTableViewRect.h},
                                 rect.w / 2, rect.h - dstTableViewRect.h};
    const Rect dtExplorerViewRect {{rect.pos.x + detailsX, rect.pos.y}, detailsWidth, rect.h};

    return {dstTableViewRect, ertTableViewRect, dtExplorerViewRect};
}

void DtsScreen::_updateTraceType()
{
    _dstTableView->traceType(this->_appState().metadata().traceType());
}

void DtsScreen::_updateViews()
{
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

void DtsScreen::_focusAndUpdateViews(View& view)
{
    this->_focus(view);
    this->_updateViews();
}

void DtsScreen::_resized()
{
    const auto viewRects = this->_viewRects();

    _dstTableView->moveAndResize(viewRects.dstTable);
    _ertTableView->moveAndResize(viewRects.ertTable);
    _dtExplorerView->moveAndResize(viewRects.dtExplorer);
    _searchCtrl.parentScreenResized(*this);
}

void DtsScreen::highlightCurDt()
{
    if (!this->_appState().hasActivePktState()) {
        _dtExplorerView->clearHighlight();
        return;
    }

    // the active trace could have changed in the meantime
    this->_updateTraceType();

    auto& activePktState = this->_appState().activePktState();

    const auto curDst = activePktState.pktIndexEntry().dst();
    const auto curEr = this->_appState().curEr();

    if (curDst) {
        _dstTableView->selectDst(curDst->id());
        _ertTableView->dst(*curDst);

        if (curEr && curEr->type()) {
            _ertTableView->selectErt(curEr->type()->id());
            _dtExplorerView->ert(*curEr->type());
            this->_focus(*_ertTableView);
        } else {
            _ertTableView->selectFirst();
            _dtExplorerView->dst(*curDst);
            this->_focus(*_dstTableView);
        }

        _dstTableView->centerSelRow();
        _ertTableView->centerSelRow();
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

    _lastErtNamePattern.reset();

    if (const auto sQuery = dynamic_cast<const ErtNameSearchQuery *>(query.get())) {
        _ertTableView->selectErt(sQuery->pattern());
        _lastErtNamePattern = sQuery->pattern();
    } else if (const auto sQuery = dynamic_cast<const ErtIdSearchQuery *>(query.get())) {
        _ertTableView->selectErt(static_cast<yactfr::TypeId>(sQuery->val()));
    }

    this->_selErtChanged();
}

void DtsScreen::_selErtChanged()
{
    this->_updateViews();
    _ertTableView->centerSelRow();
}

void DtsScreen::_handleKey(const Key key)
{
    if (key.ch() == U'+') {
        _tablesVisible = !_tablesVisible;
        this->_resized();
        _dstTableView->isVisible(_tablesVisible);
        _ertTableView->isVisible(_tablesVisible);

        if (!_tablesVisible) {
            this->_focus(*_dtExplorerView);
        }

        return;
    }

    if (!_tablesVisible) {
        // only the data type explorer is visible
        return;
    }

    const auto focusedView = this->_focusedView();

    if (const auto specialKey = key.special()) {
        switch (*specialKey) {
        case SpecialKey::ENTER:
        case SpecialKey::TAB:
            // change focus: data stream types, event record types, data types
            if (focusedView == _ertTableView.get()) {
                this->_focusAndUpdateViews(*_dtExplorerView);
            } else if (focusedView == _dtExplorerView.get()) {
                this->_focusAndUpdateViews(*_dstTableView);
            } else if (_ertTableView->isEmpty()) {
                this->_focusAndUpdateViews(*_dtExplorerView);
            } else {
                this->_focusAndUpdateViews(*_ertTableView);
            }

            break;

        case SpecialKey::LEFT:
            if (_ertTableView->isEmpty()) {
                this->_focusAndUpdateViews(*_dstTableView);
            } else {
                this->_focusAndUpdateViews(*_ertTableView);
            }

            break;

        case SpecialKey::RIGHT:
            this->_focusAndUpdateViews(*_dtExplorerView);
            break;

        default:
            break;
        }
    } else {
        const auto ch = *key.ch();

        switch (ch) {
        case '/':
        case 'g':
            if (focusedView != _ertTableView.get()) {
                break;
            }

            _searchCtrl.start([this](std::unique_ptr<const SearchQuery> query) {
                this->_searchAccepted(std::move(query));
            });

            break;

        case 'n':
            if (focusedView != _ertTableView.get() || !_lastErtNamePattern) {
                break;
            }

            _ertTableView->selectErt(*_lastErtNamePattern, true);
            this->_selErtChanged();
            break;

        default:
            break;
        }
    }
}

} // namespace jacques
