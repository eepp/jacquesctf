/*
 * SPDX-FileCopyrightText: 2022 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMON_TABLE_NAV_HPP
#define _JACQUES_INSPECT_COMMON_TABLE_NAV_HPP

#include <optional>

#include "aliases.hpp"

namespace jacques {

class TableNav
{
protected:
    enum class _Change
    {
        NONE,
        SELECTED_ROW,
        FIRST_VISIBLE_ROW,
    };

protected:
    TableNav() = default;
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

    const std::optional<Index>& _selRow() const noexcept
    {
        return _theSelRow;
    }

    Size _totalRowCount() const noexcept
    {
        return _theRowCount;
    }

    Index _firstVisibleRow() const noexcept
    {
        return _theFirstVisibleRow;
    }

private:
    Size _theRowCount = 0;
    Size _theMaxVisibleRowCount = 0;
    Index _theFirstVisibleRow = 0;
    std::optional<Index> _theSelRow;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMON_TABLE_NAV_HPP
