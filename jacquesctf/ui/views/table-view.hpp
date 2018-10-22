/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_TABLE_VIEW_HPP
#define _JACQUES_TABLE_VIEW_HPP

#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <curses.h>

#include "view.hpp"
#include "rectangle.hpp"
#include "timestamp.hpp"
#include "duration.hpp"
#include "time-ops.hpp"

namespace jacques {

class TableViewColumnDescription
{
public:
    explicit TableViewColumnDescription(const std::string& title,
                                        Size contentWidth);

    const std::string& title() const
    {
        return _title;
    }

    Size contentWidth() const
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
    enum class TextAlignment
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
    explicit TableViewCell(TextAlignment textAlignment);

public:
    virtual ~TableViewCell();

    TextAlignment textAlignment() const
    {
        return _textAlignment;
    }

    bool emphasized() const
    {
        return _emphasized;
    }

    void emphasized(const bool emphasized)
    {
        _emphasized = emphasized;
    }

    Style style() const noexcept
    {
        return _style;
    }

    void style(const Style style)
    {
        _style = style;
    }

    bool na() const
    {
        return _na;
    }

    void na(const bool na)
    {
        _na = na;
    }

private:
    const TextAlignment _textAlignment;
    bool _emphasized = false;
    Style _style = Style::NORMAL;
    bool _na = false;
};

class TextTableViewCell :
    public TableViewCell
{
public:
    explicit TextTableViewCell(TextAlignment textAlignment);

    const std::string& text() const
    {
        return _text;
    }

    void text(const std::string& text)
    {
        _text = text;
    }

private:
    std::string _text;
};

class PathTableViewCell :
    public TableViewCell
{
public:
    explicit PathTableViewCell();

    const boost::filesystem::path& path() const
    {
        return _path;
    }

    void path(const boost::filesystem::path& path)
    {
        _path = path;
    }

private:
    boost::filesystem::path _path;
};

class BoolTableViewCell :
    public TableViewCell
{
public:
    explicit BoolTableViewCell(TextAlignment textAlignment);

    bool value() const
    {
        return _value;
    }

    void value(const bool value)
    {
        _value = value;
    }

private:
    bool _value = false;
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

public:
    explicit IntTableViewCell(TextAlignment textAlignment);

    Radix radix() const
    {
        return _radix;
    }

    void radix(const Radix radix)
    {
        _radix = radix;
    }

    bool radixPrefix() const
    {
        return _radixPrefix;
    }

    void radixPrefix(const bool radixPrefix)
    {
        _radixPrefix = radixPrefix;
    }

    void sep(const bool sep)
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

class SignedIntTableViewCell :
    public IntTableViewCell
{
public:
    explicit SignedIntTableViewCell(TextAlignment textAlignment);

    long long value() const
    {
        return _value;
    }

    void value(const long long value)
    {
        _value = value;
    }

private:
    long long _value = 0;
};

class UnsignedIntTableViewCell :
    public IntTableViewCell
{
public:
    explicit UnsignedIntTableViewCell(TextAlignment textAlignment);

    unsigned long long value() const
    {
        return _value;
    }

    void value(const unsigned long long value)
    {
        _value = value;
    }

private:
    unsigned long long _value = 0;
};

class DataSizeTableViewCell :
    public TableViewCell
{
public:
    explicit DataSizeTableViewCell(utils::SizeFormatMode formatMode);

    void size(const DataSize& size) noexcept
    {
        _size = size;
    }

    const DataSize& size() const noexcept
    {
        return _size;
    }

    utils::SizeFormatMode formatMode() const
    {
        return _formatMode;
    }

    void formatMode(const utils::SizeFormatMode formatMode)
    {
        _formatMode = formatMode;
    }

private:
    DataSize _size;
    utils::SizeFormatMode _formatMode;
};

class TimestampTableViewCell :
    public TableViewCell
{
public:
    explicit TimestampTableViewCell(TimestampFormatMode tsFormatMode);

    const Timestamp& ts() const
    {
        return _ts;
    }

    void ts(const Timestamp& ts)
    {
        _ts = ts;
    }

    TimestampFormatMode formatMode() const
    {
        return _formatMode;
    }

    void formatMode(const TimestampFormatMode formatMode)
    {
        _formatMode = formatMode;
    }

private:
    Timestamp _ts;
    TimestampFormatMode _formatMode;
};

class DurationTableViewCell :
    public TableViewCell
{
public:
    explicit DurationTableViewCell(TimestampFormatMode tsFormatMode);

    const Timestamp& beginningTimestamp() const noexcept
    {
        return _beginningTimestamp;
    }

    void beginningTimestamp(const Timestamp& beginningTimestamp)
    {
        _beginningTimestamp = beginningTimestamp;
    }

    const Timestamp& endTimestamp() const noexcept
    {
        return _endTimestamp;
    }

    void endTimestamp(const Timestamp& endTimestamp)
    {
        _endTimestamp = endTimestamp;
    }

    Duration duration() const noexcept
    {
        return _endTimestamp - _beginningTimestamp;
    }

    bool cycleDiffAvailable() const noexcept
    {
        return _beginningTimestamp.frequency() == _endTimestamp.frequency();
    }

    long long cycleDiff() const noexcept
    {
        assert(this->cycleDiffAvailable());

        // FIXME: this is not accurate enough
        const auto nsDiff = static_cast<double>(this->duration().ns());
        const auto sDiff = nsDiff / 1'000'000'000.;

        return static_cast<long long>(sDiff * _beginningTimestamp.frequency());
    }

    TimestampFormatMode formatMode() const
    {
        return _formatMode;
    }

    void formatMode(const TimestampFormatMode formatMode)
    {
        _formatMode = formatMode;
    }

private:
    Timestamp _beginningTimestamp,
              _endTimestamp;
    TimestampFormatMode _formatMode;
};

class TableView :
    public View
{
public:
    void next();
    void prev();
    void pageDown();
    void pageUp();
    void centerSelectedRow(bool draw = true);
    void selectFirst();

    void selectLast()
    {
        this->_selectLast();
    }

protected:
    explicit TableView(const Rectangle& rect, const std::string& title,
                       DecorationStyle decoStyle,
                       const Stylist& stylist);
    virtual void _drawRow(Index index) = 0;
    virtual bool _hasIndex(Index index) = 0;
    virtual void _selectLast();
    void _baseIndex(Index baseIndex, bool draw = true);
    void _selectionIndex(Index index, bool draw = true);
    void _drawCells(Index index,
                    const std::vector<std::unique_ptr<TableViewCell>>& cells);
    void _drawWarningRow(Index index, const std::string& msg);
    void _columnDescriptions(std::vector<TableViewColumnDescription>&& columnDescriptions);
    void _redrawRows();
    void _redrawContent() override;
    void _isSelectionHighlightEnabled(bool isEnabled, bool draw = true);

    Index _baseIndex() const
    {
        return _baseIdx;
    }

    Index _selectionIndex() const
    {
        return _selectionIdx;
    }

    const std::vector<TableViewColumnDescription>& _columnDescriptions() const
    {
        return _columnDescrs;
    }

    Index _selectionRow() const
    {
        return this->_rowFromIndex(_selectionIdx);
    }

    Index _rowFromIndex(const Index index) const
    {
        return index - _baseIdx;
    }

    Index _selectionContentY() const
    {
        return this->_contentYFromIndex(_selectionIdx);
    }

    Index _contentYFromIndex(const Index index) const
    {
        return 1 + this->_rowFromIndex(index);
    }

    bool _indexIsSelected(const Index index) const
    {
        return index == _selectionIdx;
    }

    Size _contentSize(const Size columnCount) const
    {
        return this->contentRect().w - columnCount + 1;
    }

    void _resized() override;

private:
    void _clearRow(Index y);
    void _clearCell(const Point& pos, Size cellWidth);
    void _drawCellAlignedText(const Point& pos, Size cellWidth,
                              const char *text, Size textWidth, bool selected,
                              TableViewCell::TextAlignment alignment);
    void _drawCell(const Point& pos,
                   const TableViewColumnDescription& descr,
                   const TableViewCell& cell, bool selected);
    void _drawHeader();
    void _next(Size count);
    void _prev(Size count);
    Size _maxRowCountFromIndex(Index index);

private:
    std::vector<TableViewColumnDescription> _columnDescrs;
    Index _baseIdx = 0;
    Index _selectionIdx = 0;
    Size _visibleRowCount;
    bool _isSelectHighlightEnabled = true;
};

} // namespace jacques

#endif // _JACQUES_TABLE_VIEW_HPP
