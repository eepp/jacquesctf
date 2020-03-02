/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <array>
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <curses.h>

#include "table-view.hpp"
#include "utils.hpp"
#include "stylist.hpp"

namespace jacques {

TableView::TableView(const Rectangle& rect, const std::string& title,
                     const DecorationStyle decoStyle,
                     const Stylist& stylist) :
    View {rect, title, decoStyle, stylist},
    _visibleRowCount {this->contentRect().h - 1}
{
    assert(this->contentRect().h >= 2);
}

TableViewColumnDescription::TableViewColumnDescription(const std::string& title,
                                                       const Size contentWidth) :
    _title {title},
    _contentWidth {contentWidth}
{
}

void TableView::_columnDescriptions(std::vector<TableViewColumnDescription>&& columnDescriptions)
{
    _columnDescrs = std::move(columnDescriptions);
    this->_drawHeader();
    this->_redrawRows();
}

void TableView::_redrawContent()
{
    this->_drawHeader();
    this->_redrawRows();
}

void TableView::_baseIndex(const Index baseIndex, const bool draw)
{
    if (_baseIdx == baseIndex) {
        return;
    }

    if (!this->_hasIndex(baseIndex)) {
        return;
    }

    _baseIdx = baseIndex;

    if (_selectionIdx < _baseIdx) {
        _selectionIdx = _baseIdx;
    } else if (_selectionIdx >= _baseIdx + _visibleRowCount) {
        _selectionIdx = _baseIdx + _visibleRowCount - 1;
    }

    if (draw) {
        this->_redrawRows();
    }
}

void TableView::_selectionIndex(const Index index, const bool draw)
{
    if (!this->_hasIndex(index)) {
        return;
    }

    const auto oldSelectionIdx = _selectionIdx;

    _selectionIdx = index;

    if (index < _baseIdx) {
        this->_baseIndex(index, draw);
        return;
    } else if (index >= _baseIdx + _visibleRowCount) {
        this->_baseIndex(index - _visibleRowCount + 1, draw);
        return;
    }

    if (draw) {
        this->_drawRow(oldSelectionIdx);
        this->_drawRow(_selectionIdx);
    }
}

void TableView::_drawHeader()
{
    this->_stylist().tableViewHeader(*this);
    this->_putChar({0, 0}, ' ');

    for (Index x = 1; x < this->contentRect().w; ++x) {
        this->_appendChar(' ');
    }

    Index x = 0;

    for (auto it = std::begin(_columnDescrs); it != std::end(_columnDescrs); ++it) {
        this->_moveAndPrint({x, 0}, "%s", it->title().c_str());

        if (it == std::end(_columnDescrs) - 1) {
            break;
        }

        x += it->contentWidth();
        this->_putChar({x, 0}, ACS_VLINE);
        x += 1;
    }
}

TableViewCell::~TableViewCell()
{
}

TableViewCell::TableViewCell(const TextAlignment textAlignment) :
    _textAlignment(textAlignment)
{
}

TextTableViewCell::TextTableViewCell(const TextAlignment textAlignment) :
    TableViewCell {textAlignment}
{
}

PathTableViewCell::PathTableViewCell() :
    TableViewCell {TextAlignment::LEFT}
{
}

BoolTableViewCell::BoolTableViewCell(const TextAlignment textAlignment) :
    TableViewCell {textAlignment}
{
}

IntTableViewCell::IntTableViewCell(const TextAlignment textAlignment) :
    TableViewCell {textAlignment}
{
}

SignedIntTableViewCell::SignedIntTableViewCell(const TextAlignment textAlignment) :
    IntTableViewCell {textAlignment}
{
}

UnsignedIntTableViewCell::UnsignedIntTableViewCell(const TextAlignment textAlignment) :
    IntTableViewCell {textAlignment}
{
}

DataSizeTableViewCell::DataSizeTableViewCell(const utils::SizeFormatMode formatMode) :
    TableViewCell {TextAlignment::RIGHT},
    _formatMode {formatMode}
{
}

TimestampTableViewCell::TimestampTableViewCell(const TimestampFormatMode formatMode) :
    TableViewCell {TextAlignment::RIGHT},
    _ts {0, 1'000'000'000ULL, 0, 0},
    _formatMode {formatMode}
{
}

DurationTableViewCell::DurationTableViewCell(const TimestampFormatMode formatMode) :
    TableViewCell {TextAlignment::RIGHT},
    _beginningTimestamp {0, 1'000'000'000ULL, 0, 0},
    _endTimestamp {0, 1'000'000'000ULL, 0, 0},
    _formatMode {formatMode}
{
}

void TableView::_clearRow(const Index contentY)
{
    Index x = 0;

    this->_putChar({x, contentY}, ' ');

    for (Index x = 1; x < this->contentRect().w; ++x) {
        this->_appendChar(' ');
    }
}

void TableView::_clearCell(const Point& pos, Size cellWidth)
{
    for (Index at = 0; at < cellWidth; ++at) {
        this->_putChar({pos.x + at, pos.y}, ' ');
    }
}

void TableView::_drawCellAlignedText(const Point& contentPos,
                                     const Size cellWidth,
                                     const char * const text,
                                     Size textWidth, const bool customStyle,
                                     const TableViewCell::TextAlignment alignment)
{
    bool textMore = false;

    if (textWidth > cellWidth) {
        textWidth = cellWidth;
        textMore = true;
    }

    Index startX = contentPos.x;

    if (alignment == TableViewCell::TextAlignment::RIGHT) {
        startX = contentPos.x + cellWidth - textWidth;
    }

    // clear cell first because we might have a background color to apply
    this->_clearCell(contentPos, cellWidth);

    // write text
    for (Index at = 0; at < textWidth; ++at) {
        this->_putChar({startX + at, contentPos.y}, text[at]);
    }

    if (textMore) {
        if (customStyle) {
            this->_stylist().textMore(*this);
        }

        this->_putChar({startX + textWidth - 1, contentPos.y}, ACS_RARROW);
    }
}

void TableView::_drawCell(const Point& contentPos,
                          const TableViewColumnDescription& descr,
                          const TableViewCell& cell,
                          const bool customStyle)
{
    if (cell.na()) {
        if (customStyle) {
            this->_stylist().tableViewNaCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), "N/A", 3,
                                   customStyle, cell.textAlignment());
        return;
    }

    if (const auto rCell = dynamic_cast<const TextTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(),
                                   rCell->text().c_str(), rCell->text().size(),
                                   customStyle, cell.textAlignment());
    } else if (const auto rCell = dynamic_cast<const BoolTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewBoolCell(*this, rCell->value(),
                                               cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(),
                                   rCell->value() ? "Yes" : "No",
                                   rCell->value() ? 3 : 2,
                                   customStyle, cell.textAlignment());
    } else if (const auto dsCell = dynamic_cast<const DataSizeTableViewCell *>(&cell)) {
        const auto parts = dsCell->size().format(dsCell->formatMode(), ',');
        const char *fmt = "%s %s";
        std::array<char, 32> buf;

        if (dsCell->formatMode() == utils::SizeFormatMode::FULL_FLOOR ||
                dsCell->formatMode() == utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS) {
            fmt = "%s%4s";
        }

        std::sprintf(buf.data(), fmt, parts.first.c_str(),
                     parts.second.c_str());

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                   std::strlen(buf.data()), customStyle,
                                   cell.textAlignment());
    } else if (const auto rCell = dynamic_cast<const IntTableViewCell *>(&cell)) {
        std::array<char, 32> buf;
        const char *fmt = nullptr;

        if (rCell->radix() == IntTableViewCell::Radix::OCT) {
            if (rCell->radixPrefix()) {
                fmt = "0%llo";
            } else {
                fmt = "%llo";
            }
        } else if (rCell->radix() == IntTableViewCell::Radix::HEX) {
            if (rCell->radixPrefix()) {
                fmt = "0x%llx";
            } else {
                fmt = "%llx";
            }
        }

        if (const auto intCell = dynamic_cast<const SignedIntTableViewCell *>(&cell)) {
            assert(rCell->radix() == IntTableViewCell::Radix::DEC);

            if (intCell->sep()) {
                std::sprintf(buf.data(), "%s",
                             utils::sepNumber(intCell->value(), ',').c_str());
            } else {
                fmt = "%lld";
            }

            if (fmt) {
                std::sprintf(buf.data(), fmt, intCell->value());
            }
        } else if (auto intCell = dynamic_cast<const UnsignedIntTableViewCell *>(&cell)) {
            assert(rCell->radix() == IntTableViewCell::Radix::DEC);

            if (intCell->sep()) {
                std::sprintf(buf.data(), "%s",
                             utils::sepNumber(static_cast<long long>(intCell->value()), ',').c_str());
            } else {
                fmt = "%llu";
            }

            if (fmt) {
                std::sprintf(buf.data(), fmt, intCell->value());
            }
        } else {
            std::abort();
        }

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                   std::strlen(buf.data()), customStyle,
                                   cell.textAlignment());
    } else if (const auto rCell = dynamic_cast<const TimestampTableViewCell *>(&cell)) {
        std::array<char, 32> buf;

        switch (rCell->formatMode()) {
        case TimestampFormatMode::LONG:
        case TimestampFormatMode::SHORT:
            rCell->ts().format(buf.data(), buf.size(), rCell->formatMode());

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_drawCellAlignedText(contentPos, descr.contentWidth(),
                                       buf.data(), std::strlen(buf.data()),
                                       customStyle, cell.textAlignment());
            break;

        case TimestampFormatMode::NS_FROM_ORIGIN:
        {
            const auto parts = utils::formatNs(rCell->ts().nsFromOrigin(), ',');
            const auto partsWidth = parts.first.size() + parts.second.size() + 4;
            const auto startPos = Point {
                contentPos.x + descr.contentWidth() - partsWidth, contentPos.y
            };

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_clearCell(contentPos, descr.contentWidth());
            this->_moveAndPrint(startPos, "%s,", parts.first.c_str());

            if (customStyle) {
                this->_stylist().tableViewTsCellNsPart(*this, cell.emphasized());
            }

            this->_print("%s", parts.second.c_str());

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_print(" ns");
            break;
        }

        case TimestampFormatMode::CYCLES:
        {
            const auto str = utils::sepNumber(rCell->ts().cycles(), ',') + " cc";

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_drawCellAlignedText(contentPos, descr.contentWidth(),
                                       str.c_str(), str.size(), customStyle,
                                       cell.textAlignment());
            break;
        }
        }
    } else if (const auto rCell = dynamic_cast<const DurationTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        std::array<char, 32> buf;

        buf[0] = '\0';

        switch (rCell->formatMode()) {
        case TimestampFormatMode::LONG:
        case TimestampFormatMode::SHORT:
        {
            char *fmtBuf = buf.data();
            auto fmtSize = buf.size();

            if (rCell->isNegative()) {
                buf[0] = '-';
                ++fmtBuf;
                --fmtSize;
            }

            rCell->absDuration().format(fmtBuf, fmtSize);
            break;
        }

        case TimestampFormatMode::NS_FROM_ORIGIN:
        {
            const auto parts = utils::formatNs(rCell->absDuration().ns(), ',');
            const auto partsWidth = parts.first.size() + parts.second.size() + 4;
            const auto startPos = Point {
                contentPos.x + descr.contentWidth() - partsWidth -
                (rCell->isNegative() ? 1 : 0),
                contentPos.y
            };

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_clearCell(contentPos, descr.contentWidth());
            this->_moveCursor(startPos);

            if (rCell->isNegative()) {
                this->_appendChar('-');
            }

            this->_safePrint("%s,", parts.first.c_str());

            if (customStyle) {
                this->_stylist().tableViewTsCellNsPart(*this, cell.emphasized());
            }

            this->_safePrint("%s", parts.second.c_str());

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_safePrint(" ns");
            break;
        }

        case TimestampFormatMode::CYCLES:
            if (!rCell->cycleDiffAvailable()) {
                if (customStyle) {
                    this->_stylist().tableViewNaCell(*this, cell.emphasized());
                }

                this->_drawCellAlignedText(contentPos, descr.contentWidth(),
                                           "N/A", 3, customStyle,
                                           cell.textAlignment());
                break;
            }

            std::sprintf(buf.data(), "%s%s cc",
                         rCell->isNegative() ? "-" : "",
                         utils::sepNumber(rCell->absCycleDiff(), ',').c_str());
            break;

        default:
            break;
        }

        if (std::strlen(buf.data()) > 0) {
            this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                       std::strlen(buf.data()), customStyle,
                                       cell.textAlignment());
        }
    } else if (const auto pCell = dynamic_cast<const PathTableViewCell *>(&cell)) {
        std::string dirName, filename;

        assert(!pCell->path().empty());
        std::tie(dirName, filename) = utils::formatPath(pCell->path(),
                                                        descr.contentWidth());

        auto curPos = contentPos;

        if (!dirName.empty()) {
            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, false);
            }

            this->_drawCellAlignedText(curPos, descr.contentWidth(),
                                       dirName.c_str(), dirName.size(),
                                       customStyle,
                                       TableViewCell::TextAlignment::LEFT);
            curPos.x += dirName.size();
            this->_drawCellAlignedText(curPos, descr.contentWidth(),
                                       "/", 1, customStyle,
                                       TableViewCell::TextAlignment::LEFT);
            curPos.x += 1;
        }

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(curPos, descr.contentWidth(),
                                   filename.c_str(), filename.size(),
                                   customStyle,
                                   TableViewCell::TextAlignment::LEFT);
    } else {
        std::abort();
    }
}

static inline
Stylist::TableViewCellStyle stylistTvcStyleFromTvcStyle(const TableViewCell::Style style)
{
    switch (style) {
    case TableViewCell::Style::NORMAL:
        return Stylist::TableViewCellStyle::NORMAL;

    case TableViewCell::Style::WARNING:
        return Stylist::TableViewCellStyle::WARNING;

    case TableViewCell::Style::ERROR:
        return Stylist::TableViewCellStyle::ERROR;

    default:
        std::abort();
    }
}

void TableView::_drawCells(const Index index,
                           const std::vector<std::unique_ptr<TableViewCell>>& cells)
{
    assert(cells.size() == _columnDescrs.size());

    const auto selected = _isSelectHighlightEnabled &&
                          this->_indexIsSelected(index);
    Index x = 0;
    const auto y = _contentYFromIndex(index);

    // set style to clear row initially
    if (selected) {
        this->_stylist().tableViewSelection(*this);
    } else {
        this->_stylist().tableViewCell(*this);
    }

    this->_clearRow(y);

    for (Index column = 0; column < cells.size(); ++column) {
        const auto& descr = _columnDescrs[column];
        const auto& cell = *cells[column];
        auto cellStylistTvcStyle = stylistTvcStyleFromTvcStyle(cell.style());

        if (selected) {
            this->_stylist().tableViewSelection(*this, cellStylistTvcStyle);
        } else {
            this->_stylist().tableViewCell(*this, cellStylistTvcStyle);
        }

        this->_drawCell({x, y}, _columnDescrs[column], cell,
                        !selected && cell.style() == TableViewCell::Style::NORMAL);
        x += descr.contentWidth();

        if (column == cells.size() - 1) {
            break;
        }

        if (selected) {
            this->_stylist().tableViewSelectionSep(*this,
                                                   Stylist::TableViewCellStyle::NORMAL);
        } else {
            this->_stylist().tableViewSep(*this);
        }

        this->_putChar({x, y}, ACS_VLINE);
        x += 1;
    }
}

void TableView::_drawWarningRow(const Index index, const std::string& msg)
{
    const auto selected = this->_indexIsSelected(index);
    const Index x = (this->contentRect().w - msg.size()) / 2;
    const auto y = _contentYFromIndex(index);

    if (selected) {
        this->_stylist().tableViewSelection(*this, Stylist::TableViewCellStyle::WARNING);
    } else {
        this->_stylist().tableViewCell(*this, Stylist::TableViewCellStyle::WARNING);
    }

    this->_clearRow(y);
    this->_moveAndPrint({x, y}, "%s", msg.c_str());
}

void TableView::_redrawRows()
{
    Index idx = _baseIdx;

    for (idx = _baseIdx; idx < _baseIdx + _visibleRowCount; ++idx) {
        if (this->_hasIndex(idx)) {
            this->_drawRow(idx);
        } else {
            break;
        }
    }

    this->_stylist().tableViewSep(*this);

    for (; idx < _baseIdx + _visibleRowCount; ++idx) {
        this->_clearRow(this->_contentYFromIndex(idx));
    }

    this->_hasMoreTop(_baseIdx > 0);
    this->_hasMoreBottom(this->_hasIndex(_baseIdx + _visibleRowCount));
}

void TableView::_resized()
{
    _visibleRowCount = this->contentRect().h - 1;
    _selectionIdx = std::min(_selectionIdx,
                             _baseIdx + _visibleRowCount - 1);
}

void TableView::_next(Size count)
{
    assert(count > 0);

    const auto oldSelectionIndex = _selectionIdx;

    while (true) {
        this->_selectionIndex(_selectionIdx + count);

        if (_selectionIdx != oldSelectionIndex) {
            break;
        }

        --count;

        if (count == 0) {
            break;
        }
    }
}

void TableView::_prev(Size count)
{
    if (_selectionIdx == 0) {
        return;
    }

    if (count > _selectionIdx) {
        count = _selectionIdx;
    }

    this->_selectionIndex(_selectionIdx - count);
}

Size TableView::_maxRowCountFromIndex(const Index index)
{
    Size rows = 0;

    while (rows < _visibleRowCount) {
        if (!this->_hasIndex(index + rows)) {
            break;
        }

        ++rows;
    }

    return rows;
}

void TableView::next()
{
    this->_next(1);
}

void TableView::prev()
{
    this->_prev(1);
}

void TableView::pageDown()
{
    const auto idealNewBaseIndex = _baseIdx + _visibleRowCount;
    auto effectiveNewBaseIndex = idealNewBaseIndex;

    while (!this->_hasIndex(effectiveNewBaseIndex)) {
        --effectiveNewBaseIndex;
    }

    if (idealNewBaseIndex != effectiveNewBaseIndex) {
        this->_selectionIndex(effectiveNewBaseIndex);
    } else {
        const auto oldSelectionIndex = _selectionIdx;

        this->_baseIndex(effectiveNewBaseIndex);
        this->_selectionIndex(oldSelectionIndex + _visibleRowCount);
    }
}

void TableView::pageUp()
{
    Index newBaseIndex = 0;
    Index newSelectionIndex = 0;

    if (_baseIdx >= _visibleRowCount) {
        newBaseIndex = _baseIdx - _visibleRowCount;
        newSelectionIndex = _selectionIdx - _visibleRowCount;
    }

    this->_baseIndex(newBaseIndex);
    this->_selectionIndex(newSelectionIndex);
}

void TableView::centerSelectedRow(const bool draw)
{
    if (_baseIdx == 0 && this->_maxRowCountFromIndex(0) < _visibleRowCount) {
        // all rows already visible
        return;
    }

    const auto newBaseIndex = static_cast<long long>(_selectionIdx -
                                                     _visibleRowCount / 2);

    if (newBaseIndex < 0) {
        // row is in the first half
        this->_baseIndex(0);
        return;
    }

    const auto rowCountsFromNewBaseIdx = this->_maxRowCountFromIndex(newBaseIndex);

    if (rowCountsFromNewBaseIdx < _visibleRowCount) {
        // row is in the last half
        this->_baseIndex(static_cast<Index>(newBaseIndex) +
                         rowCountsFromNewBaseIdx - _visibleRowCount, draw);
        return;
    }

    this->_baseIndex(static_cast<Index>(newBaseIndex), draw);
}

void TableView::selectFirst()
{
    this->_selectionIndex(0);
}

void TableView::_selectLast()
{
}

void TableView::_isSelectionHighlightEnabled(const bool isEnabled,
                                             const bool draw)
{
    if (_isSelectHighlightEnabled == isEnabled) {
        return;
    }

    _isSelectHighlightEnabled = isEnabled;

    if (draw) {
        this->_redrawRows();
    }
}

} // namespace jacques
