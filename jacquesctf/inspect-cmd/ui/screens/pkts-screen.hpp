/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "../views/pkt-table-view.hpp"
#include "../search-ctrl.hpp"
#include "screen.hpp"
#include "../cycle-wheel.hpp"
#include "data/data-len.hpp"

namespace jacques {

class PktsScreen final :
    public Screen
{
public:
    explicit PktsScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                        State& state);

protected:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<PktTableView> _ptView;
    SearchCtrl _searchCtrl;
    std::unique_ptr<const SearchQuery> _lastQuery;
    CycleWheel<TsFmtMode> _tsFmtModeWheel;
    CycleWheel<utils::LenFmtMode> _dataLenFmtModeWheel;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_PKTS_SCREEN_HPP
