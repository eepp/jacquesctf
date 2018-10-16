/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TYPES_SCREEN_HPP
#define _JACQUES_DATA_TYPES_SCREEN_HPP

#include <functional>
#include <tuple>

#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "event-record-type-table-view.hpp"
#include "data-stream-type-table-view.hpp"
#include "data-type-explorer-view.hpp"
#include "screen.hpp"
#include "interactive.hpp"
#include "search-controller.hpp"

namespace jacques {

class DataTypesScreen :
    public Screen
{
public:
    explicit DataTypesScreen(const Rectangle& rect, const Config& cfg,
                             std::shared_ptr<const Stylist> stylist,
                             std::shared_ptr<State> state);
    void highlightCurrentDataType();
    void clearHighlight();

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;
    std::tuple<Rectangle, Rectangle, Rectangle> _viewRects() const;
    void _updateViews();

private:
    std::unique_ptr<EventRecordTypeTableView> _ertTableView;
    std::unique_ptr<DataStreamTypeTableView> _dstTableView;
    std::unique_ptr<DataTypeExplorerView> _dtExplorerView;
    SearchController _searchController;
    View *_focusedView;
    bool _tablesVisible = true;
    std::unique_ptr<const EventRecordTypeNameSearchQuery> _lastQuery;
};

} // namespace jacques

#endif // _JACQUES_DATA_TYPES_SCREEN_HPP
