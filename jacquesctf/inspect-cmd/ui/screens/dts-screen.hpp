/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP

#include <tuple>

#include "../stylist.hpp"
#include "../views/ert-table-view.hpp"
#include "../views/dst-table-view.hpp"
#include "../views/dt-explorer-view.hpp"
#include "screen.hpp"
#include "../search-ctrl.hpp"

namespace jacques {

class DtsScreen final :
    public Screen
{
public:
    explicit DtsScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                       ScreenHost& host);
    void highlightCurDt();
    void clearHighlight();

private:
    void _resized() override;
    void _handleKey(Key key) override;
    void _focusedViewHandledKey(Key key) override;
    void _visibilityChanged() override;
    std::tuple<Rect, Rect, Rect> _viewRects() const;
    void _updateViews();

    /*
     * Selects the event record type which the accepted query `query`,
     * if any, matches.
     */
    void _searchAccepted(std::unique_ptr<const SearchQuery> query);

private:
    std::unique_ptr<ErtTableView> _ertTableView;
    std::unique_ptr<DstTableView> _dstTableView;
    std::unique_ptr<DtExplorerView> _dtExplorerView;
    SearchCtrl _searchController;
    bool _tablesVisible = true;
    std::unique_ptr<const ErtNameSearchQuery> _lastQuery;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
