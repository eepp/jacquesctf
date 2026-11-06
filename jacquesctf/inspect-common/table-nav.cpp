/*
 * SPDX-FileCopyrightText: 2022 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <algorithm>

#include "table-nav.hpp"

namespace jacques {

void TableNav::_updateCounts(const Size rowCount, const Size maxVisibleRowCount)
{
    if (rowCount != _theRowCount) {
        _theFirstVisibleRow = 0;
        _theSelRow.reset();
    }

    _theRowCount = rowCount;
    _theMaxVisibleRowCount = maxVisibleRowCount;
    _theFirstVisibleRow = std::min(_theFirstVisibleRow, this->_maxFirstVisibleRow());
}

bool TableNav::_firstVisibleRow(const Index firstVisibleRow)
{
    assert(firstVisibleRow <= this->_maxFirstVisibleRow());

    if (firstVisibleRow == _theFirstVisibleRow) {
        return false;
    }

    _theFirstVisibleRow = firstVisibleRow;
    return true;
}

TableNav::_Change TableNav::_selFirstRow(const bool ensureVisible)
{
    return this->_selRow(0, ensureVisible);
}

TableNav::_Change TableNav::_selLastRow(const bool ensureVisible)
{
    if (_theRowCount == 0) {
        return _Change::NONE;
    }

    return this->_selRow(_theRowCount - 1, ensureVisible);
}

TableNav::_Change TableNav::_selNextRow(const Size steps, const bool ensureVisible)
{
    if (!_theSelRow) {
        return _Change::NONE;
    }

    return this->_selRow(std::min(*_theSelRow + steps, _theRowCount - 1), ensureVisible);
}

TableNav::_Change TableNav::_selPrevRow(const Size steps, const bool ensureVisible)
{
    if (!_theSelRow) {
        return _Change::NONE;
    }

    return this->_selRow(*_theSelRow - std::min(steps, *_theSelRow), ensureVisible);
}

TableNav::_Change TableNav::_selRow(const Index row, const bool ensureVisible)
{
    if (_theRowCount == 0) {
        return _Change::NONE;
    }

    if (_theSelRow && row == *_theSelRow) {
        return _Change::NONE;
    }

    _theSelRow = row;

    if (ensureVisible && !this->_rowIsVisible(row)) {
        if (row < _theFirstVisibleRow) {
            _theFirstVisibleRow = row;
        } else {
            _theFirstVisibleRow = row - _theMaxVisibleRowCount + 1;
        }

        return _Change::FIRST_VISIBLE_ROW;
    }

    return _Change::SELECTED_ROW;
}

void TableNav::_removeSel()
{
    _theSelRow.reset();
}

bool TableNav::_goUp(const Size steps)
{
    return this->_firstVisibleRow(_theFirstVisibleRow - std::min(steps, _theFirstVisibleRow));
}

bool TableNav::_goDown(const Size steps)
{
    return this->_firstVisibleRow(std::min(_theFirstVisibleRow + steps,
                                           this->_maxFirstVisibleRow()));
}

bool TableNav::_pageUp()
{
    return this->_goUp(_theMaxVisibleRowCount);
}

bool TableNav::_pageDown()
{
    return this->_goDown(_theMaxVisibleRowCount);
}

bool TableNav::_showFirstPage()
{
    return this->_firstVisibleRow(0);
}

bool TableNav::_showLastPage()
{
    return this->_firstVisibleRow(this->_maxFirstVisibleRow());
}

bool TableNav::_centerOnRow(const Index row)
{
    assert(row < _theRowCount);

    const auto halfMaxVisibleRowCount = _theMaxVisibleRowCount / 2;

    return this->_firstVisibleRow(std::min(row - std::min(row, halfMaxVisibleRowCount),
                                           this->_maxFirstVisibleRow()));
}

bool TableNav::_centerOnSelRow()
{
    if (!_theSelRow) {
        return false;
    }

    return this->_centerOnRow(*_theSelRow);
}

Index TableNav::_maxFirstVisibleRow() const noexcept
{
    return this->_isSinglePage() ? 0 : _theRowCount - _theMaxVisibleRowCount;
}

Size TableNav::_visibleRowCount() const noexcept
{
    return std::min(_theRowCount, _theMaxVisibleRowCount);
}

Index TableNav::_lastVisibleRow() const noexcept
{
    assert(_theRowCount > 0);
    return _theFirstVisibleRow + this->_visibleRowCount() - 1;
}

bool TableNav::_rowIsVisible(const Index row) const noexcept
{
    assert(row < _theRowCount);
    return row >= _theFirstVisibleRow && row < _theFirstVisibleRow + this->_visibleRowCount();
}

Index TableNav::_yIndexFromVisibleRow(const Index row) const noexcept
{
    assert(this->_rowIsVisible(row));
    return row - _theFirstVisibleRow;
}

Index TableNav::_rowFromYIndex(const Index yIndex) const noexcept
{
    assert(_theFirstVisibleRow + yIndex < _theRowCount);
    return _theFirstVisibleRow + yIndex;
}

bool TableNav::_isSinglePage() const noexcept
{
    return _theRowCount <= _theMaxVisibleRowCount;
}

} // namespace jacques
