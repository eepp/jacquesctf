/*
 * Copyright (C) 2022 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMON_COMMON_INSPECT_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_COMMON_COMMON_INSPECT_TABLE_VIEW_HPP

#include <cassert>
#include <utility>
#include <tuple>
#include <boost/optional.hpp>

#include "aliases.hpp"

namespace jacques {

class CommonInspectTableView
{
protected:
    enum class _Change
    {
        NONE,
        SELECTED_ROW,
        FIRST_VISIBLE_ROW,
    };

protected:
    CommonInspectTableView() = default;
    void _updateCounts(Size rowCount, Size maxVisibleRowCount);
    bool _firstVisibleRow(Index firstVisibleRow);
    _Change _selFirstRow(bool ensureVisible = false);
    _Change _selLastRow(bool ensureVisible = false);
    _Change _selNextRow(Size steps = 1, bool ensureVisible = false);
    _Change _selPrevRow(Size steps = 1, bool ensureVisible = false);
    _Change _selRow(Index row, bool ensureVisible = false);
    void _removeSel();
    bool _goUp(Size steps = 1);
    bool _goDown(Size steps = 1);
    bool _pageUp();
    bool _pageDown();
    bool _showFirstPage();
    bool _showLastPage();
    bool _centerOnRow(Index row);
    bool _centerOnSelRow();
    Index _maxFirstVisibleRow() const noexcept;
    Size _visibleRowCount() const noexcept;
    Index _lastVisibleRow() const noexcept;
    bool _rowIsVisible(Index row) const noexcept;
    Index _yIndexFromVisibleRow(Index row) const noexcept;
    Index _rowFromYIndex(Index yIndex) const noexcept;
    bool _isSinglePage() const noexcept;

    bool _rowIsSel(const Index row) const noexcept
    {
        return _theSelRow && row == *_theSelRow;
    }

    Size _maxVisibleRowCount() const noexcept
    {
        return _theMaxVisibleRowCount;
    }

    const boost::optional<Index>& _selRow() const noexcept
    {
        return _theSelRow;
    }

    Index _firstVisibleRow() const noexcept
    {
        return _theFirstVisibleRow;
    }

private:
    template <typename FuncT>
    _Change _selRow(const Size steps, const bool ensureVisible, FuncT&& func)
    {
        assert(_theRowCount > 0);

        auto change = _Change::NONE;

        for (Index step = 0; step < steps; ++step) {
            const auto stepChange = this->_selRow(func(), ensureVisible);

            if (stepChange == _Change::NONE) {
                // nothing more to do
                return change;
            }

            if (static_cast<int>(stepChange) > static_cast<int>(change)) {
                // more important change
                change = stepChange;
            }
        }

        return change;
    }

private:
    Size _theRowCount = 0;
    Size _theMaxVisibleRowCount = 0;
    Index _theFirstVisibleRow = 0;
    boost::optional<Index> _theSelRow;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMON_COMMON_INSPECT_TABLE_VIEW_HPP
