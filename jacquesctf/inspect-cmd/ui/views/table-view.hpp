/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <boost/filesystem.hpp>

#include "inspect-common/common-inspect-table-view.hpp"
#include "view.hpp"
#include "../stylist.hpp"
#include "../rect.hpp"
#include "data/ts.hpp"
#include "data/data-len.hpp"
#include "utils.hpp"

namespace jacques {

/*
 * Base class of a table view.
 *
 * A concrete table view declares its columns once with _cols(): each
 * column has a cell function which returns the cell to draw for a
 * given row.
 *
 * The table view only shows the columns which fit, dropping
 * non-required columns from the right, and gives the leftover width to
 * the expanding column (the last shown column if none).
 */
class TableView :
    public View,
    public CommonInspectTableView
{
public:
    void next();
    void prev();
    void pageDown();
    void pageUp();
    void showFirstPage();
    void showLastPage();
    void centerSelRow();
    void selectFirst();
    void selectLast();
    void highlightSel(bool highlight);
    void tsFmtMode(TsFmtMode tsFmtMode);
    void dataLenFmtMode(utils::LenFmtMode dataLenFmtMode);

protected:
    using _CellStyle = Stylist::TableViewCellStyle;

    /*
     * Cell to draw: a sequence of segments, each one having its own
     * role, and a style for the whole cell.
     *
     * The segments only get their own style when the cell style is
     * `_CellStyle::NORMAL` and the row isn't selected.
     */
    struct _Cell final
    {
        enum class SegRole
        {
            // regular text, emphasized if the column is
            TEXT,

            // regular text, never emphasized
            PLAIN,

            // value not available
            NA,

            // accent (nanosecond part of a timestamp, for example)
            ACCENT,
        };

        struct Seg final
        {
            std::string text;
            SegRole role;
        };

        _Cell() = default;

        explicit _Cell(std::string text, const SegRole role = SegRole::TEXT) :
            segs {{std::move(text), role}}
        {
        }

        std::vector<Seg> segs;
        _CellStyle style = _CellStyle::NORMAL;
    };

    // column flags
    static constexpr unsigned int _COL_LEFT = 1U << 0;     // left-align (default: right)
    static constexpr unsigned int _COL_EMPH = 1U << 1;     // emphasized
    static constexpr unsigned int _COL_EXPAND = 1U << 2;   // takes the leftover width
    static constexpr unsigned int _COL_REQUIRED = 1U << 3; // never dropped

    class _Col final
    {
    public:
        using CellFunc = std::function<_Cell (Index)>;

    public:
        /*
         * `width` is the minimum content width and `flags` is a
         * bitwise OR of `_COL_*` flags.
         */
        explicit _Col(std::string title, Size width, unsigned int flags, CellFunc cellFunc);

        const std::string& title() const noexcept
        {
            return _title;
        }

        Size width() const noexcept
        {
            return _width;
        }

        unsigned int flags() const noexcept
        {
            return _flags;
        }

        // cell to draw for the row `row`
        _Cell cell(const Index row) const
        {
            return _cellFunc(row);
        }

    private:
        std::string _title;
        Size _width;
        unsigned int _flags;
        CellFunc _cellFunc;
    };

protected:
    explicit TableView(const Rect& rect, std::string_view title, DecorationStyle decoStyle,
                       const Stylist& stylist);

    virtual Size _rowCount() = 0;
    using CommonInspectTableView::_updateCounts;
    void _updateCounts();
    void _cols(std::vector<_Col>&& cols);
    void _redrawContent() override;
    void _selectRow(Index row);
    void _removeSel();
    void _resized() override;

    // current content width of the shown column `col`
    Size _colWidth(const Index col) const noexcept
    {
        return _colWidths[col];
    }

    static _Cell _createNaCell();
    static _Cell _createTextCell(std::string_view text);
    static _Cell _createUIntCell(unsigned long long val, bool sep = false);
    static _Cell _createPathCell(const boost::filesystem::path& path, Size width);
    _Cell _createDataLenCell(const DataLen& len) const;
    _Cell _createTsCell(const Ts& ts) const;
    _Cell _createDurationCell(const Ts& beginTs, const Ts& endTs) const;

private:
    // "seconds,nanoseconds ns" cell, the nanosecond part being accented
    static _Cell _createNsCell(const char *sign, long long ns);

    void _fitCols();
    void _drawHeader();
    void _drawRows();
    void _drawRow(Index row);
    void _drawCell(const Point& contentPos, Size width, const _Col& col, const _Cell& cell,
                   bool sel);
    void _applySegStyle(_Cell::SegRole role, bool emph);
    void _clearRow(Index y);
    void _clearCell(const Point& pos, Size cellWidth);

    Index _contentYFromVisibleRow(const Index row) const noexcept
    {
        return 1 + this->_yIndexFromVisibleRow(row);
    }

private:
    // all the declared columns
    std::vector<_Col> _theCols;

    // content widths of the shown columns (first ones of `_theCols`)
    std::vector<Size> _colWidths;

    bool _highlightSel = true;
    TsFmtMode _tsFmtMode = TsFmtMode::LONG;
    utils::LenFmtMode _dataLenFmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
