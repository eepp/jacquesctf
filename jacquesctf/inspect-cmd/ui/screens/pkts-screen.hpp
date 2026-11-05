/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../views/pkt-table-view.hpp"
#include "../search-ctrl.hpp"
#include "screen.hpp"

namespace jacques {

class PktsScreen final :
    public Screen
{
public:
    /*
     * `renderFrameFunc` renders the whole terminal (see `SearchCtrl`).
     */
    explicit PktsScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                        InspectCmdState& appState, SearchCtrl::RenderFrameFunc renderFrameFunc);

protected:
    void _resized() override;
    KeyHandlingReaction _handleKey(Key key) override;

private:
    std::unique_ptr<PktTableView> _ptView;
    SearchCtrl _searchCtrl;
    std::unique_ptr<const SearchQuery> _lastQuery;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
