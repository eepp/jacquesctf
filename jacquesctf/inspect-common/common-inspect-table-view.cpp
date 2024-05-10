/*
 * Copyright (C) 2022 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "common-inspect-table-view.hpp"

namespace jacques {

void CommonInspectTableView::_updateCounts(const Size rowCount, const Size maxVisibleRowCount)
{
    if (rowCount != _theRowCount) {
        _theFirstVisibleRow = 0;
        _theSelRow.reset();
    }

    _theRowCount = rowCount;
    _theMaxVisibleRowCount = maxVisibleRowCount;

    if (_theRowCount > 0) {
        _theFirstVisibleRow = std::min(_theFirstVisibleRow, this->_maxFirstVisibleRow());
    }
}

bool CommonInspectTableView::_firstVisibleRow(const Index firstVisibleRow)
{
    assert(_theRowCount > 0);
    assert(firstVisibleRow <= this->_maxFirstVisibleRow());

    if (firstVisibleRow == _theFirstVisibleRow) {
        return false;
    }

    _theFirstVisibleRow = firstVisibleRow;
    return true;
}

CommonInspectTableView::_Change CommonInspectTableView::_selFirstRow(const bool ensureVisible)
{
    assert(_theRowCount > 0);
    return this->_selRow(0, ensureVisible);
}

CommonInspectTableView::_Change CommonInspectTableView::_selLastRow(const bool ensureVisible)
{
    assert(_theRowCount > 0);
    return this->_selRow(_theRowCount - 1, ensureVisible);
}

CommonInspectTableView::_Change CommonInspectTableView::_selNextRow(const Size steps,
                                                                    const bool ensureVisible)
{
    if (!_theSelRow) {
        return _Change::NONE;
    }

    return this->_selRow(steps, ensureVisible, [this] {
        return std::min(*_theSelRow + 1, _theRowCount - 1);
    });
}

CommonInspectTableView::_Change CommonInspectTableView::_selPrevRow(const Size steps,
                                                                    const bool ensureVisible)
{
    if (!_theSelRow) {
        return _Change::NONE;
    }

    return this->_selRow(steps, ensureVisible, [this] {
        return *_theSelRow == 0 ? 0 : *_theSelRow - 1;
    });
}

CommonInspectTableView::_Change CommonInspectTableView::_selRow(const Index row,
                                                                const bool ensureVisible)
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

void CommonInspectTableView::_removeSel()
{
    _theSelRow.reset();
}

bool CommonInspectTableView::_goUp(const Size steps)
{
    assert(_theRowCount > 0);

    auto changed = false;

    for (Index step = 0; step < steps; ++step) {
        if (_theFirstVisibleRow == 0) {
            return changed;
        }

        --_theFirstVisibleRow;
        changed = true;
    }

    return changed;
}

bool CommonInspectTableView::_goDown(const Size steps)
{
    assert(_theRowCount > 0);

    auto changed = false;

    for (Index step = 0; step < steps; ++step) {
        if (_theFirstVisibleRow == this->_maxFirstVisibleRow()) {
            return changed;
        }

        ++_theFirstVisibleRow;
        changed = true;
    }

    return changed;
}

bool CommonInspectTableView::_pageUp()
{
    return this->_goUp(_theMaxVisibleRowCount);
}

bool CommonInspectTableView::_pageDown()
{
    return this->_goDown(_theMaxVisibleRowCount);
}

bool CommonInspectTableView::_showFirstPage()
{
    if (_theRowCount == 0) {
        return false;
    }

    return this->_firstVisibleRow(0);
}

bool CommonInspectTableView::_showLastPage()
{
    if (_theRowCount == 0) {
        return false;
    }

    return this->_firstVisibleRow(this->_maxFirstVisibleRow());
}

bool CommonInspectTableView::_centerOnRow(const Index row)
{
    assert(_theRowCount > 0);

    if (this->_isSinglePage()) {
        return this->_firstVisibleRow(0);
    }

    const auto halfMaxVisibleRowCount = _theMaxVisibleRowCount / 2;

    if (row <= halfMaxVisibleRowCount) {
        return this->_firstVisibleRow(0);
    }

    return this->_firstVisibleRow(std::min(row - halfMaxVisibleRowCount,
                                           this->_maxFirstVisibleRow()));
}

bool CommonInspectTableView::_centerOnSelRow()
{
    if (!_theSelRow) {
        return false;
    }

    return this->_centerOnRow(*_theSelRow);
}

Index CommonInspectTableView::_maxFirstVisibleRow() const noexcept
{
    assert(_theRowCount > 0);

    if (this->_isSinglePage()) {
        return 0;
    }

    return _theRowCount - _theMaxVisibleRowCount;
}

Size CommonInspectTableView::_visibleRowCount() const noexcept
{
    return _theRowCount <= _theMaxVisibleRowCount ? _theRowCount : _theMaxVisibleRowCount;
}

Index CommonInspectTableView::_lastVisibleRow() const noexcept
{
    assert(_theRowCount > 0);
    return _theFirstVisibleRow + this->_visibleRowCount() - 1;
}

bool CommonInspectTableView::_rowIsVisible(const Index row) const noexcept
{
    assert(_theRowCount > 0);
    assert(row < _theRowCount);
    return row >= _theFirstVisibleRow && row < _theFirstVisibleRow + this->_visibleRowCount();
}

Index CommonInspectTableView::_yIndexFromVisibleRow(const Index row) const noexcept
{
    assert(this->_rowIsVisible(row));
    return row - _theFirstVisibleRow;
}

Index CommonInspectTableView::_rowFromYIndex(const Index yIndex) const noexcept
{
    assert(_theRowCount > 0);
    assert(_theFirstVisibleRow + yIndex < _theRowCount);
    return _theFirstVisibleRow + yIndex;
}

bool CommonInspectTableView::_isSinglePage() const noexcept
{
    assert(_theRowCount > 0);
    return this->_visibleRowCount() < _theMaxVisibleRowCount;
}

} // namespace jacques
