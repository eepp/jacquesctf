/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP

#include "../stylist.hpp"
#include "../views/pkt-table-view.hpp"
#include "../search-ctrl.hpp"
#include "screen.hpp"

namespace jacques {

class PktsScreen final :
    public Screen
{
public:
    explicit PktsScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                        ScreenHost& host);

private:
    void _resized() override;
    void _handleKey(Key key) override;

private:
    std::unique_ptr<PktTableView> _ptView;
    SearchCtrl _searchCtrl;
    std::unique_ptr<const SearchQuery> _lastQuery;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
