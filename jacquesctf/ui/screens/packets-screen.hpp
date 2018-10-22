/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKETS_SCREEN_HPP
#define _JACQUES_PACKETS_SCREEN_HPP

#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "packet-table-view.hpp"
#include "search-controller.hpp"
#include "screen.hpp"
#include "interactive.hpp"
#include "cycle-wheel.hpp"
#include "data-size.hpp"

namespace jacques {

class PacketsScreen :
    public Screen
{
public:
    explicit PacketsScreen(const Rectangle& rect, const Config& cfg,
                           const Stylist& stylist, State& state);

protected:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<PacketTableView> _ptView;
    SearchController _searchController;
    std::unique_ptr<const SearchQuery> _lastQuery;
    CycleWheel<TimestampFormatMode> _tsFormatModeWheel;
    CycleWheel<utils::SizeFormatMode> _dsFormatModeWheel;
};

} // namespace jacques

#endif // _JACQUES_PACKETS_SCREEN_HPP
