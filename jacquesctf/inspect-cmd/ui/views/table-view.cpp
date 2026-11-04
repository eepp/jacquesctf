/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <algorithm>
#include <numeric>

#include "table-view.hpp"
#include "utils.hpp"
#include "data/duration.hpp"
#include "data/time-ops.hpp"
#include "../stylist.hpp"
#include "../term.hpp"

namespace jacques {

TableView::TableView(const Rect& rect, const std::string_view title,
                     const DecorationStyle decoStyle, const Stylist& stylist) :
    View {rect, title, decoStyle, stylist}
{
    assert(this->contentRect().h >= 2);
}

TableView::_Col::_Col(std::string title, const Size width, const unsigned int flags,
                      CellFunc cellFunc) :
    _title {std::move(title)},
    _width {width},
    _flags {flags},
    _cellFunc {std::move(cellFunc)}
{
}

void TableView::_cols(std::vector<_Col>&& cols)
{
    _theCols = std::move(cols);
    this->_fitCols();
    this->_updateCounts();
}

void TableView::_fitCols()
{
    // available content width for `count` columns (one separator between two columns)
    const auto availWidth = [this](const Size count) -> Size {
        const Size w = this->contentRect().w;

        return count == 0 || w + 1 <= count ? 0 : w - count + 1;
    };

    auto count = _theCols.size();
    auto widthSum = std::accumulate(_theCols.begin(), _theCols.end(), Size {0},
                                    [](const auto sum, const auto& col) {
        return sum + col.width();
    });

    // drop non-required columns from the right until they all fit
    while (count > 0 && widthSum > availWidth(count) &&
            !(_theCols[count - 1].flags() & _COL_REQUIRED)) {
        --count;
        widthSum -= _theCols[count].width();
    }

    _colWidths.clear();

    for (Index col = 0; col < count; ++col) {
        _colWidths.push_back(_theCols[col].width());
    }

    if (count == 0) {
        return;
    }

    // give the leftover width to the expanding column (last one if none)
    auto expandCol = count - 1;

    for (Index col = 0; col < count; ++col) {
        if (_theCols[col].flags() & _COL_EXPAND) {
            expandCol = col;
            break;
        }
    }

    const auto otherWidth = widthSum - _colWidths[expandCol];
    const auto avail = availWidth(count);

    _colWidths[expandCol] = avail > otherWidth ? avail - otherWidth : 0;
}

void TableView::_redrawContent()
{
    // the derived class must call _cols() from its constructor
    assert(!_theCols.empty());

    this->_drawHeader();
    this->_drawRows();
}

void TableView::_drawHeader()
{
    this->_stylist().tableViewHeader(*this);
    this->_clearRow(0);

    Index x = 0;

    for (Index col = 0; col < _colWidths.size(); ++col) {
        this->_putStr({x, 0}, term::strFittingPrefix(_theCols[col].title(), _colWidths[col]));
        x += _colWidths[col];

        if (col == _colWidths.size() - 1) {
            break;
        }

        this->_putChar({x, 0}, glyphs::VLINE);
        x += 1;
    }
}

void TableView::_clearRow(const Index contentY)
{
    this->_putChar({0, contentY}, ' ');

    for (Index x = 1; x < this->contentRect().w; ++x) {
        this->_appendChar(' ');
    }
}

void TableView::_clearCell(const Point& pos, const Size cellWidth)
{
    for (Index at = 0; at < cellWidth; ++at) {
        this->_putChar({pos.x + at, pos.y}, ' ');
    }
}

void TableView::_applySegStyle(const _Cell::SegRole role, const bool emph)
{
    switch (role) {
    case _Cell::SegRole::TEXT:
        this->_stylist().tableViewTextCell(*this, emph);
        break;

    case _Cell::SegRole::PLAIN:
        this->_stylist().tableViewTextCell(*this, false);
        break;

    case _Cell::SegRole::NA:
        this->_stylist().tableViewNaCell(*this, emph);
        break;

    case _Cell::SegRole::ACCENT:
        this->_stylist().tableViewTsCellNsPart(*this, emph);
        break;
    }
}

void TableView::_drawCell(const Point& contentPos, const Size width, const _Col& col,
                          const _Cell& cell, const bool sel)
{
    // clear with the cell style first because it might have a background color
    if (sel) {
        this->_stylist().tableViewSel(*this, cell.style);
    } else {
        this->_stylist().tableViewCell(*this, cell.style);
    }

    this->_clearCell(contentPos, width);

    if (width == 0) {
        return;
    }

    const auto segStyles = !sel && cell.style == _CellStyle::NORMAL;
    const auto emph = static_cast<bool>(col.flags() & _COL_EMPH);
    Size textWidth = 0;

    for (auto& seg : cell.segs) {
        textWidth += term::strWidth(seg.text);
    }

    /*
     * If the text doesn't fit, then write what fits before the last
     * column, which is for the "more" indicator.
     *
     * The written text can occupy one column less than that when the
     * next character is a double-width one: the cell is already clear.
     */
    const auto fits = textWidth <= width;
    auto x = contentPos.x;
    auto availWidth = fits ? width : width - 1;

    if (fits && !(col.flags() & _COL_LEFT)) {
        x += width - textWidth;
    }

    for (auto& seg : cell.segs) {
        const auto text = term::strFittingPrefix(seg.text, availWidth);

        if (!text.empty()) {
            if (segStyles) {
                this->_applySegStyle(seg.role, emph);
            }

            const auto segWidth = term::strWidth(text);

            this->_putStr({x, contentPos.y}, text);
            x += segWidth;
            availWidth -= segWidth;
        }

        if (text.size() < seg.text.size()) {
            break;
        }
    }

    if (!fits) {
        if (segStyles) {
            this->_stylist().textMore(*this);
        }

        this->_putChar({contentPos.x + width - 1, contentPos.y}, glyphs::RARROW);
    }
}

void TableView::_drawRow(const Index row)
{
    const auto sel = this->_rowIsSel(row) && _highlightSel;
    const auto y = this->_contentYFromVisibleRow(row);
    Index x = 0;

    // set style to clear row initially
    if (sel) {
        this->_stylist().tableViewSel(*this);
    } else {
        this->_stylist().tableViewCell(*this);
    }

    this->_clearRow(y);

    for (Index colIndex = 0; colIndex < _colWidths.size(); ++colIndex) {
        const auto& col = _theCols[colIndex];
        const auto width = _colWidths[colIndex];

        this->_drawCell({x, y}, width, col, col.cell(row), sel);
        x += width;

        if (colIndex == _colWidths.size() - 1) {
            break;
        }

        if (sel) {
            this->_stylist().tableViewSelSep(*this);
        } else {
            this->_stylist().tableViewSep(*this);
        }

        this->_putChar({x, y}, glyphs::VLINE);
        x += 1;
    }
}

TableView::_Cell TableView::_createNaCell()
{
    return _Cell {"N/A", _Cell::SegRole::NA};
}

TableView::_Cell TableView::_createTextCell(const std::string_view text)
{
    return _Cell {utils::escapeStr(text)};
}

TableView::_Cell TableView::_createUIntCell(const unsigned long long val, const bool sep)
{
    return _Cell {sep ? utils::sepNumber(val, ',') : std::to_string(val)};
}

TableView::_Cell TableView::_createPathCell(const boost::filesystem::path& path, const Size width)
{
    assert(!path.empty());

    auto [dirName, filename] = _formatPath(utils::escapeStr(path.string()), width);
    _Cell cell;

    if (!dirName.empty()) {
        cell.segs.push_back({std::move(dirName) + '/', _Cell::SegRole::PLAIN});
    }

    cell.segs.push_back({std::move(filename), _Cell::SegRole::TEXT});
    return cell;
}

TableView::_Cell TableView::_createDataLenCell(const DataLen& len) const
{
    const auto [qty, unit] = len.format(_dataLenFmtMode, ',');

    if (_dataLenFmtMode == utils::LenFmtMode::FULL_FLOOR ||
            _dataLenFmtMode == utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS) {
        // right-align the unit within four columns
        return _Cell {qty + std::string(unit.size() < 4 ? 4 - unit.size() : 0, ' ') + unit};
    }

    return _Cell {qty + ' ' + unit};
}

TableView::_Cell TableView::_createNsCell(const char * const sign, const long long ns)
{
    const auto [sPart, nsPart] = utils::formatNs(ns, ',');
    _Cell cell;

    cell.segs = {
        {sign + sPart + ',', _Cell::SegRole::TEXT},
        {nsPart, _Cell::SegRole::ACCENT},
        {" ns", _Cell::SegRole::TEXT},
    };

    return cell;
}

TableView::_Cell TableView::_createTsCell(const Ts& ts) const
{
    switch (ts.effectiveFmtMode(_tsFmtMode)) {
    case TsFmtMode::LONG:
    case TsFmtMode::SHORT:
        return _Cell {ts.format(_tsFmtMode)};

    case TsFmtMode::NS_FROM_ORIGIN:
        return this->_createNsCell("", ts.nsFromOrigin());

    case TsFmtMode::CYCLES:
        return _Cell {utils::sepNumber(ts.cycles(), ',') + " cc"};
    }

    std::abort();
}

TableView::_Cell TableView::_createDurationCell(const Ts& beginTs, const Ts& endTs) const
{
    const auto sign = beginTs > endTs ? "-" : "";
    const auto absDuration = std::max(beginTs, endTs) - std::min(beginTs, endTs);

    switch (_tsFmtMode) {
    case TsFmtMode::LONG:
    case TsFmtMode::SHORT:
        return _Cell {sign + absDuration.format()};

    case TsFmtMode::NS_FROM_ORIGIN:
        return this->_createNsCell(sign, absDuration.ns());

    case TsFmtMode::CYCLES:
    {
        if (beginTs.frequency() != endTs.frequency()) {
            return this->_createNaCell();
        }

        // FIXME: this is not accurate enough
        const auto sDiff = static_cast<double>(absDuration.ns()) / 1'000'000'000.;
        const auto cycleDiff = static_cast<long long>(sDiff * beginTs.frequency());

        return _Cell {sign + utils::sepNumber(cycleDiff, ',') + " cc"};
    }
    }

    std::abort();
}

void TableView::_updateCounts()
{
    this->_updateCounts(this->_rowCount(), this->contentRect().h - 1);
    this->_invalidate();
}

void TableView::_drawRows()
{
    if (this->_rowCount() == 0) {
        this->_stylist().tableViewCell(*this);

        for (Index yIndex = 0; yIndex < this->contentRect().h; ++yIndex) {
            this->_clearRow(yIndex);
        }

        return;
    }

    for (auto row = this->_firstVisibleRow();
            row < this->_firstVisibleRow() + this->_visibleRowCount(); ++row) {
        this->_drawRow(row);
    }

    this->_stylist().tableViewSep(*this);

    for (auto yIndex = this->_contentYFromVisibleRow(this->_lastVisibleRow()) + 1;
            yIndex < this->contentRect().h; ++yIndex) {
        this->_clearRow(yIndex);
    }

    this->_hasMoreTop(!this->_rowIsVisible(0));
    this->_hasMoreBottom(!this->_rowIsVisible(this->_rowCount() - 1));
}

void TableView::_selectRow(const Index row)
{
    this->_selRow(row, true);
    this->_invalidate();
}

void TableView::_removeSel()
{
    CommonInspectTableView::_removeSel();
    this->_invalidate();
}

void TableView::_resized()
{
    this->_updateCounts();
    this->_fitCols();
}

void TableView::tsFmtMode(const TsFmtMode tsFmtMode)
{
    _tsFmtMode = tsFmtMode;
    this->_invalidate();
}

void TableView::dataLenFmtMode(const utils::LenFmtMode dataLenFmtMode)
{
    _dataLenFmtMode = dataLenFmtMode;
    this->_invalidate();
}

void TableView::next()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_selNextRow(1, true);
    this->_invalidate();
}

void TableView::prev()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_selPrevRow(1, true);
    this->_invalidate();
}

void TableView::pageDown()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_pageDown();

    if (this->_selRow()) {
        this->_selRow(std::min(*this->_selRow() + this->_maxVisibleRowCount(),
                               this->_rowCount() - 1));
    }

    this->_invalidate();
}

void TableView::pageUp()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_pageUp();

    if (this->_selRow()) {
        if (*this->_selRow() < this->_maxVisibleRowCount()) {
            this->_selFirstRow();
        } else {
            this->_selRow(*this->_selRow() - this->_maxVisibleRowCount());
        }
    }

    this->_invalidate();
}

void TableView::showFirstPage()
{
    this->_showFirstPage();
    this->_invalidate();
}

void TableView::showLastPage()
{
    this->_showLastPage();
    this->_invalidate();
}

void TableView::centerSelRow()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_centerOnSelRow();
    this->_invalidate();
}

void TableView::selectFirst()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_selFirstRow(true);
    this->_invalidate();
}

void TableView::selectLast()
{
    if (this->_rowCount() == 0) {
        return;
    }

    this->_selLastRow(true);
    this->_invalidate();
}

void TableView::highlightSel(const bool highlight)
{
    _highlightSel = highlight;
    this->_invalidate();
}

} // namespace jacques
