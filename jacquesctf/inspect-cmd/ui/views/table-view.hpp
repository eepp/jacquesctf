/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <boost/filesystem.hpp>

#include "inspect-common/table-nav.hpp"
#include "view.hpp"
#include "../stylist.hpp"
#include "../rect.hpp"
#include "data/ts.hpp"
#include "data/data-len.hpp"

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
    public TableNav
{
public:
    void centerSelRow();
    void selectFirst();
    void selectLast();
    void highlightSel(bool highlight);

protected:
    // style of a whole cell
    enum class _CellStyle
    {
        NORMAL,
        WARNING,
        ERROR,
    };

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

            // subtle accent (nanosecond part of a timestamp, for example)
            SOFT_ACCENT,

            // dimmed (UTC offset of a timestamp, for example)
            DIM,

            // unit (`MiB`, `ns`, `cc`)
            UNIT,
        };

        struct Seg final
        {
            std::string text;
            SegRole role;

            /*
             * Within a `_COL_DIM_SAME` column, dimmed when the segment
             * at the same index within the cell of the previous visible
             * row also
             * has this flag and the same text, and when all the
             * preceding segments having this flag are also dimmed (for
             * example, the time of a timestamp isn't dimmed when its
             * date changes).
             *
             * The first visible row never has such dimmed segments.
             */
            bool dimIfSameAsPrev = false;
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

    /*
     * Timestamp column: its title shows the local UTC offset when it's
     * the same for all the timestamps (see _createTsCell()).
     */
    static constexpr unsigned int _COL_TS = 1U << 4;

    /*
     * Dims the segments which are the same as the ones of the previous
     * visible row (see `_Cell::Seg::dimIfSameAsPrev`).
     *
     * This only makes sense when the rows are sorted by this column.
     */
    static constexpr unsigned int _COL_DIM_SAME = 1U << 5;

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
    /*
     * The data length and timestamp cells follow the format modes of
     * `appState` (see InspectCmdState::tsFmtMode() and
     * InspectCmdState::dataLenFmtMode()).
     *
     * A derived table view which shows such cells must invalidate
     * itself on `Message::FMT_MODES_CHANGED`.
     */
    explicit TableView(const Rect& rect, std::string_view title, DecorationStyle decoStyle,
                       const Stylist& stylist, const InspectCmdState& appState);

    virtual Size _rowCount() = 0;
    using TableNav::_updateCounts;
    void _updateCounts();
    void _cols(std::vector<_Col>&& cols);
    void _redrawContent() override;
    void _selectRow(Index row);
    void _removeSel();
    void _resized() override;

    /*
     * Handles Up, Down, Page Up, Page Down, Home, End (selection), and
     * `c` (center the selected row).
     */
    bool _handleKey(Key key) override;

    // current content width of the shown column `col`
    Size _colWidth(const Index col) const noexcept
    {
        return _colWidths[col];
    }

    // minimum content width of a timestamp column (see `_COL_TS`)
    Size _tsColWidth() const noexcept
    {
        return _theTsColWidth;
    }

    static _Cell _createNaCell();
    static _Cell _createTextCell(std::string_view text);
    static _Cell _createUIntCell(unsigned long long val, bool sep = false);
    static _Cell _createPathCell(const boost::filesystem::path& path, Size width);
    _Cell _createDataLenCell(const DataLen& len) const;

    /*
     * Timestamp cell, for a `_COL_TS` column.
     *
     * The cell only shows the local UTC offset when it differs from
     * the one which the column title shows.
     *
     * Within a `_COL_DIM_SAME` column, the date and time parts are
     * dimmed when they're the same as the ones of the previous visible
     * row (see `_Cell::Seg::dimIfSameAsPrev`).
     */
    _Cell _createTsCell(const Ts& ts) const;
    _Cell _createDurationCell(const Ts& beginTs, const Ts& endTs) const;

private:
    // `qty` followed with `unit` (including any leading space) cell
    static _Cell _createQtyCell(std::string qty, std::string unit);

    // "seconds,nanoseconds ns" cell, the nanosecond part being softly accented
    static _Cell _createNsCell(const char *sign, long long ns);

    // appends a dimmed `utcOffset` segment to `cell` if needed (see _createTsCell())
    void _tryAppendUtcOffsetSeg(_Cell& cell, const std::string& utcOffset) const;

    void _selNext();
    void _selPrev();
    void _selNextPage();
    void _selPrevPage();
    void _fitCols();
    void _drawHeader();
    void _drawRows();
    std::vector<_Cell> _drawRow(Index row, const std::vector<_Cell>& prevCells);
    static void _dimSameSegs(_Cell& cell, const _Cell& prevCell);
    void _drawCell(const Point& contentPos, Size width, const _Col& col, const _Cell& cell,
                   bool sel);
    void _applySegStyle(_Cell::SegRole role, bool emph);
    static Stylist::Role _cellRole(_CellStyle style, bool sel) noexcept;

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

    // for the format modes
    const InspectCmdState *_theAppState;

    // local UTC offset of all the timestamps, if it's the same
    std::optional<std::string> _tsUtcOffset;

    // minimum content width of a timestamp column
    Size _theTsColWidth;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
