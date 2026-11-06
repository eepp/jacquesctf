/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP

#include <optional>
#include <string>

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
    struct _ViewRects final
    {
        Rect dstTable;
        Rect ertTable;
        Rect dtExplorer;
    };

private:
    void _resized() override;
    void _handleKey(Key key) override;
    void _focusedViewHandledKey(Key key) override;
    void _visibilityChanged() override;
    _ViewRects _viewRects() const;

    /*
     * Makes the data stream type table view show the data stream types
     * of the active trace.
     */
    void _updateTraceType();

    void _updateViews();

    /*
     * Focuses `view` and then updates the views, as the data type
     * explorer shows the selection of the focused table view.
     */
    void _focusAndUpdateViews(View& view);

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

    // pattern of the last event record type name query, if any
    std::optional<std::string> _lastErtNamePattern;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
