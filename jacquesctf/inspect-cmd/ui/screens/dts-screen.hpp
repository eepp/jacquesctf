/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP

#include <functional>
#include <tuple>

#include "aliases.hpp"
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
    explicit DtsScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                       InspectCmdState& appState);
    void highlightCurDt();
    void clearHighlight();

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(Key key) override;
    void _visibilityChanged() override;
    std::tuple<Rect, Rect, Rect> _viewRects() const;
    void _updateViews();

private:
    std::unique_ptr<ErtTableView> _ertTableView;
    std::unique_ptr<DstTableView> _dstTableView;
    std::unique_ptr<DtExplorerView> _dtExplorerView;
    SearchCtrl _searchController;
    View *_focusedView;
    bool _tablesVisible = true;
    std::unique_ptr<const ErtNameSearchQuery> _lastQuery;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_DTS_SCREEN_HPP
