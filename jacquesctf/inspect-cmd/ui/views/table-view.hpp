/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP

#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <curses.h>

#include "view.hpp"
#include "../rect.hpp"
#include "data/ts.hpp"
#include "data/duration.hpp"
#include "data/time-ops.hpp"

namespace jacques {

class TableViewColumnDescr final
{
public:
    explicit TableViewColumnDescr(std::string title, Size contentWidth);
    TableViewColumnDescr(const TableViewColumnDescr&) = default;
    TableViewColumnDescr& operator=(const TableViewColumnDescr&) = default;

    const std::string& title() const noexcept
    {
        return _title;
    }

    Size contentWidth() const noexcept
    {
        return _contentWidth;
    }

private:
    std::string _title;
    Size _contentWidth;
};

class TableViewCell
{
public:
    enum class TextAlign
    {
        LEFT,
        RIGHT,
    };

public:
    enum class Style
    {
        NORMAL,
        WARNING,
        ERROR,
    };

protected:
    explicit TableViewCell(TextAlign textAlign) noexcept;

public:
    virtual ~TableViewCell() = default;

    TextAlign textAlign() const noexcept
    {
        return _textAlign;
    }

    bool emphasized() const noexcept
    {
        return _emphasized;
    }

    void emphasized(const bool emphasized) noexcept
    {
        _emphasized = emphasized;
    }

    Style style() const noexcept
    {
        return _style;
    }

    void style(const Style style) noexcept
    {
        _style = style;
    }

    bool na() const noexcept
    {
        return _na;
    }

    void na(const bool na) noexcept
    {
        _na = na;
    }

private:
    const TextAlign _textAlign;
    bool _emphasized = false;
    Style _style = Style::NORMAL;
    bool _na = false;
};

class TextTableViewCell final :
    public TableViewCell
{
public:
    explicit TextTableViewCell(TextAlign textAlign);

    const std::string& text() const noexcept
    {
        return _text;
    }

    void text(const std::string& text)
    {
        _text = utils::escapeStr(text);
    }

private:
    std::string _text;
};

class PathTableViewCell final :
    public TableViewCell
{
public:
    explicit PathTableViewCell();

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    void path(const boost::filesystem::path& path)
    {
        _path = utils::escapeStr(path.string());
    }

private:
    boost::filesystem::path _path;
};

class BoolTableViewCell final :
    public TableViewCell
{
public:
    explicit BoolTableViewCell(TextAlign textAlign);

    bool val() const noexcept
    {
        return _val;
    }

    void val(const bool val) noexcept
    {
        _val = val;
    }

private:
    bool _val = false;
};

class IntTableViewCell :
    public TableViewCell
{
public:
    enum class Radix
    {
        OCT,
        DEC,
        HEX,
    };

protected:
    explicit IntTableViewCell(TextAlign textAlign) noexcept;

public:
    Radix radix() const noexcept
    {
        return _radix;
    }

    void radix(const Radix radix) noexcept
    {
        _radix = radix;
    }

    bool radixPrefix() const noexcept
    {
        return _radixPrefix;
    }

    void radixPrefix(const bool radixPrefix) noexcept
    {
        _radixPrefix = radixPrefix;
    }

    void sep(const bool sep) noexcept
    {
        _sep = sep;
    }

    bool sep() const noexcept
    {
        return _sep;
    }

private:
    Radix _radix = Radix::DEC;
    bool _radixPrefix = true;
    bool _sep = false;
};

class SIntTableViewCell final :
    public IntTableViewCell
{
public:
    explicit SIntTableViewCell(TextAlign textAlign) noexcept;

    long long val() const noexcept
    {
        return _val;
    }

    void val(const long long val) noexcept
    {
        _val = val;
    }

private:
    long long _val = 0;
};

class UIntTableViewCell final :
    public IntTableViewCell
{
public:
    explicit UIntTableViewCell(TextAlign textAlign) noexcept;

    unsigned long long val() const noexcept
    {
        return _val;
    }

    void val(const unsigned long long val) noexcept
    {
        _val = val;
    }

private:
    unsigned long long _val = 0;
};

class DataLenTableViewCell final :
    public TableViewCell
{
public:
    explicit DataLenTableViewCell(utils::LenFmtMode fmtMode) noexcept;

    void len(const DataLen& len) noexcept
    {
        _len = len;
    }

    const DataLen& len() const noexcept
    {
        return _len;
    }

    utils::LenFmtMode fmtMode() const noexcept
    {
        return _fmtMode;
    }

    void fmtMode(const utils::LenFmtMode fmtMode) noexcept
    {
        _fmtMode = fmtMode;
    }

private:
    DataLen _len;
    utils::LenFmtMode _fmtMode;
};

class TsTableViewCell final :
    public TableViewCell
{
public:
    explicit TsTableViewCell(TsFmtMode tsFmtMode) noexcept;

    const Ts& ts() const noexcept
    {
        return _ts;
    }

    void ts(const Ts& ts)
    {
        _ts = ts;
    }

    TsFmtMode fmtMode() const noexcept
    {
        return _fmtMode;
    }

    void fmtMode(const TsFmtMode fmtMode) noexcept
    {
        _fmtMode = fmtMode;
    }

private:
    Ts _ts;
    TsFmtMode _fmtMode;
};

class DurationTableViewCell final :
    public TableViewCell
{
public:
    explicit DurationTableViewCell(TsFmtMode tsFmtMode) noexcept;

    const Ts& beginTs() const noexcept
    {
        return _beginTs;
    }

    void beginTs(const Ts& beginTs) noexcept
    {
        _beginTs = beginTs;
    }

    const Ts& endTs() const noexcept
    {
        return _endTs;
    }

    void endTs(const Ts& endTs) noexcept
    {
        _endTs = endTs;
    }

    Duration absDuration() const noexcept
    {
        return std::max(_beginTs, _endTs) - std::min(_beginTs, _endTs);
    }

    bool isNegative() const noexcept
    {
        return _beginTs > _endTs;
    }

    bool cycleDiffAvailable() const noexcept
    {
        return _beginTs.frequency() == _endTs.frequency();
    }

    long long absCycleDiff() const noexcept
    {
        assert(this->cycleDiffAvailable());

        // FIXME: this is not accurate enough
        const auto nsDiff = static_cast<double>(this->absDuration().ns());
        const auto sDiff = nsDiff / 1'000'000'000.;

        return static_cast<long long>(sDiff * _beginTs.frequency());
    }

    TsFmtMode fmtMode() const noexcept
    {
        return _fmtMode;
    }

    void fmtMode(const TsFmtMode fmtMode) noexcept
    {
        _fmtMode = fmtMode;
    }

private:
    Ts _beginTs;
    Ts _endTs;
    TsFmtMode _fmtMode;
};

class TableView :
    public View
{
public:
    void next();
    void prev();
    void pageDown();
    void pageUp();
    void centerSelRow(bool draw = true);
    void selectFirst();

    void selectLast()
    {
        this->_selectLast();
    }

protected:
    explicit TableView(const Rect& rect, const std::string& title, DecorationStyle decoStyle,
                       const Stylist& stylist);

    virtual void _drawRow(Index index) = 0;
    virtual bool _hasIndex(Index index) = 0;
    virtual void _selectLast();
    void _baseIndex(Index baseIndex, bool draw = true);
    void _selIndex(Index index, bool draw = true);
    void _drawCells(Index index, const std::vector<std::unique_ptr<TableViewCell>>& cells);
    void _drawWarningRow(Index index, const std::string& msg);
    void _colDescrs(std::vector<TableViewColumnDescr>&& columnDescriptions);
    void _redrawRows();
    void _redrawContent() override;
    void _isSelHighlightEnabled(bool isEnabled, bool draw = true);

    Index _baseIndex() const noexcept
    {
        return _theBaseIndex;
    }

    Index _selIndex() const noexcept
    {
        return _theSelIndex;
    }

    const std::vector<TableViewColumnDescr>& _colDescrs() const noexcept
    {
        return _theColDescrs;
    }

    Index _selRow() const noexcept
    {
        return this->_rowFromIndex(_theSelIndex);
    }

    Index _rowFromIndex(const Index index) const noexcept
    {
        return index - _theBaseIndex;
    }

    Index _selContentY() const noexcept
    {
        return this->_contentYFromIndex(_theSelIndex);
    }

    Index _contentYFromIndex(const Index index) const noexcept
    {
        return 1 + this->_rowFromIndex(index);
    }

    bool _indexIsSel(const Index index) const noexcept
    {
        return index == _theSelIndex;
    }

    Size _contentSize(const Size columnCount) const noexcept
    {
        return this->contentRect().w - columnCount + 1;
    }

    void _resized() override;

private:
    void _clearRow(Index y);
    void _clearCell(const Point& pos, Size cellWidth);

    void _drawCellAlignedText(const Point& pos, Size cellWidth, const char *text, Size textWidth,
                              bool customStyle, TableViewCell::TextAlign align);

    void _drawCell(const Point& pos, const TableViewColumnDescr& descr, const TableViewCell& cell,
                   bool customStyle);

    void _drawHeader();
    void _next(Size count);
    void _prev(Size count);
    Size _maxRowCountFromIndex(Index index);

private:
    std::vector<TableViewColumnDescr> _theColDescrs;
    Index _theBaseIndex = 0;
    Index _theSelIndex = 0;
    Size _visibleRowCount;
    bool _isSelHighlightEnabledMemb = true;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_TABLE_VIEW_HPP
