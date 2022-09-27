/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <stdio.h>
#include <array>
#include <cinttypes>
#include <cstring>
#include <cstdio>
#include <curses.h>

#include "table-view.hpp"
#include "utils.hpp"
#include "../stylist.hpp"

namespace jacques {

TableView::TableView(const Rect& rect, const std::string& title, const DecorationStyle decoStyle,
                     const Stylist& stylist) :
    View {rect, title, decoStyle, stylist},
    _visibleRowCount {this->contentRect().h - 1}
{
    assert(this->contentRect().h >= 2);
}

TableViewColumnDescr::TableViewColumnDescr(std::string title, const Size contentWidth) :
    _title {std::move(title)},
    _contentWidth {contentWidth}
{
}

void TableView::_colDescrs(std::vector<TableViewColumnDescr>&& colDescrs)
{
    _theColDescrs = std::move(colDescrs);
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
    if (_theBaseIndex == baseIndex) {
        return;
    }

    if (!this->_hasIndex(baseIndex)) {
        return;
    }

    _theBaseIndex = baseIndex;

    if (_theSelIndex < _theBaseIndex) {
        _theSelIndex = _theBaseIndex;
    } else if (_theSelIndex >= _theBaseIndex + _visibleRowCount) {
        _theSelIndex = _theBaseIndex + _visibleRowCount - 1;
    }

    if (draw) {
        this->_redrawRows();
    }
}

void TableView::_selIndex(const Index index, const bool draw)
{
    if (!this->_hasIndex(index)) {
        return;
    }

    const auto oldSelIndex = _theSelIndex;

    _theSelIndex = index;

    if (index < _theBaseIndex) {
        this->_baseIndex(index, draw);
        return;
    } else if (index >= _theBaseIndex + _visibleRowCount) {
        this->_baseIndex(index - _visibleRowCount + 1, draw);
        return;
    }

    if (draw) {
        this->_drawRow(oldSelIndex);
        this->_drawRow(_theSelIndex);
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

    for (auto it = _theColDescrs.begin(); it != _theColDescrs.end(); ++it) {
        this->_moveAndPrint({x, 0}, "%s", it->title().c_str());

        if (it == _theColDescrs.end() - 1) {
            break;
        }

        x += it->contentWidth();
        this->_putChar({x, 0}, ACS_VLINE);
        x += 1;
    }
}

TableViewCell::TableViewCell(const TextAlign textAlign) noexcept :
    _textAlign(textAlign)
{
}

TextTableViewCell::TextTableViewCell(const TextAlign textAlign) :
    TableViewCell {textAlign}
{
}

PathTableViewCell::PathTableViewCell() :
    TableViewCell {TextAlign::LEFT}
{
}

BoolTableViewCell::BoolTableViewCell(const TextAlign textAlign) :
    TableViewCell {textAlign}
{
}

IntTableViewCell::IntTableViewCell(const TextAlign textAlign) noexcept :
    TableViewCell {textAlign}
{
}

SIntTableViewCell::SIntTableViewCell(const TextAlign textAlign) noexcept :
    IntTableViewCell {textAlign}
{
}

UIntTableViewCell::UIntTableViewCell(const TextAlign textAlign) noexcept :
    IntTableViewCell {textAlign}
{
}

DataLenTableViewCell::DataLenTableViewCell(const utils::LenFmtMode fmtMode) noexcept :
    TableViewCell {TextAlign::RIGHT},
    _fmtMode {fmtMode}
{
}

TsTableViewCell::TsTableViewCell(const TsFmtMode fmtMode) noexcept :
    TableViewCell {TextAlign::RIGHT},
    _ts {0, 1'000'000'000ULL, 0, 0},
    _fmtMode {fmtMode}
{
}

DurationTableViewCell::DurationTableViewCell(const TsFmtMode fmtMode) noexcept :
    TableViewCell {TextAlign::RIGHT},
    _beginTs {0, 1'000'000'000ULL, 0, 0},
    _endTs {0, 1'000'000'000ULL, 0, 0},
    _fmtMode {fmtMode}
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

void TableView::_drawCellAlignedText(const Point& contentPos, const Size cellWidth,
                                     const char * const text, Size textWidth,
                                     const bool customStyle, const TableViewCell::TextAlign align)
{
    bool textMore = false;

    if (textWidth > cellWidth) {
        textWidth = cellWidth;
        textMore = true;
    }

    Index startX = contentPos.x;

    if (align == TableViewCell::TextAlign::RIGHT) {
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

void TableView::_drawCell(const Point& contentPos, const TableViewColumnDescr& descr,
                          const TableViewCell& cell, const bool customStyle)
{
    if (cell.na()) {
        if (customStyle) {
            this->_stylist().tableViewNaCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), "N/A", 3, customStyle,
                                   cell.textAlign());
        return;
    }

    if (const auto rCell = dynamic_cast<const TextTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), rCell->text().c_str(),
                                   rCell->text().size(), customStyle, cell.textAlign());
    } else if (const auto rCell = dynamic_cast<const BoolTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewBoolCell(*this, rCell->val(), cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), rCell->val() ? "Yes" : "No",
                                   rCell->val() ? 3 : 2, customStyle, cell.textAlign());
    } else if (const auto dsCell = dynamic_cast<const DataLenTableViewCell *>(&cell)) {
        const auto parts = dsCell->len().format(dsCell->fmtMode(), ',');
        const char *fmt = "%s %s";
        std::array<char, 32> buf;

        if (dsCell->fmtMode() == utils::LenFmtMode::FULL_FLOOR ||
                dsCell->fmtMode() == utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS) {
            fmt = "%s%4s";
        }

        std::sprintf(buf.data(), fmt, parts.first.c_str(), parts.second.c_str());

        auto fh = fopen("/tmp/meow", "wb");

        fwrite(parts.first.c_str(), 1, parts.first.size() + 1, fh);
        fclose(fh);

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                   std::strlen(buf.data()), customStyle, cell.textAlign());
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

        if (const auto intCell = dynamic_cast<const SIntTableViewCell *>(&cell)) {
            assert(rCell->radix() == IntTableViewCell::Radix::DEC);

            if (intCell->sep()) {
                std::sprintf(buf.data(), "%s", utils::sepNumber(intCell->val(), ',').c_str());
            } else {
                fmt = "%lld";
            }

            if (fmt) {
                std::sprintf(buf.data(), fmt, intCell->val());
            }
        } else if (auto intCell = dynamic_cast<const UIntTableViewCell *>(&cell)) {
            assert(rCell->radix() == IntTableViewCell::Radix::DEC);

            if (intCell->sep()) {
                std::sprintf(buf.data(), "%s",
                             utils::sepNumber(static_cast<long long>(intCell->val()), ',').c_str());
            } else {
                fmt = "%llu";
            }

            if (fmt) {
                std::sprintf(buf.data(), fmt, intCell->val());
            }
        } else {
            std::abort();
        }

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                   std::strlen(buf.data()), customStyle, cell.textAlign());
    } else if (const auto rCell = dynamic_cast<const TsTableViewCell *>(&cell)) {
        std::array<char, 32> buf;

        switch (rCell->fmtMode()) {
        case TsFmtMode::LONG:
        case TsFmtMode::SHORT:
            rCell->ts().format(buf.data(), buf.size(), rCell->fmtMode());

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                       std::strlen(buf.data()), customStyle, cell.textAlign());
            break;

        case TsFmtMode::NS_FROM_ORIGIN:
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

        case TsFmtMode::CYCLES:
        {
            const auto str = utils::sepNumber(rCell->ts().cycles(), ',') + " cc";

            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, cell.emphasized());
            }

            this->_drawCellAlignedText(contentPos, descr.contentWidth(), str.c_str(), str.size(),
                                       customStyle, cell.textAlign());
            break;
        }
        }
    } else if (const auto rCell = dynamic_cast<const DurationTableViewCell *>(&cell)) {
        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        std::array<char, 32> buf;

        buf[0] = '\0';

        switch (rCell->fmtMode()) {
        case TsFmtMode::LONG:
        case TsFmtMode::SHORT:
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

        case TsFmtMode::NS_FROM_ORIGIN:
        {
            const auto parts = utils::formatNs(rCell->absDuration().ns(), ',');
            const auto partsWidth = parts.first.size() + parts.second.size() + 4;
            const auto startPos = Point {
                contentPos.x + descr.contentWidth() - partsWidth - (rCell->isNegative() ? 1 : 0),
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

        case TsFmtMode::CYCLES:
            if (!rCell->cycleDiffAvailable()) {
                if (customStyle) {
                    this->_stylist().tableViewNaCell(*this, cell.emphasized());
                }

                this->_drawCellAlignedText(contentPos, descr.contentWidth(), "N/A", 3, customStyle,
                                           cell.textAlign());
                break;
            }

            std::sprintf(buf.data(), "%s%s cc", rCell->isNegative() ? "-" : "",
                         utils::sepNumber(rCell->absCycleDiff(), ',').c_str());
            break;

        default:
            break;
        }

        if (std::strlen(buf.data()) > 0) {
            this->_drawCellAlignedText(contentPos, descr.contentWidth(), buf.data(),
                                       std::strlen(buf.data()), customStyle, cell.textAlign());
        }
    } else if (const auto pCell = dynamic_cast<const PathTableViewCell *>(&cell)) {
        std::string dirName, filename;

        assert(!pCell->path().empty());
        std::tie(dirName, filename) = utils::formatPath(pCell->path(), descr.contentWidth());

        auto curPos = contentPos;

        if (!dirName.empty()) {
            if (customStyle) {
                this->_stylist().tableViewTextCell(*this, false);
            }

            this->_drawCellAlignedText(curPos, descr.contentWidth(), dirName.c_str(),
                                       dirName.size(), customStyle, TableViewCell::TextAlign::LEFT);
            curPos.x += dirName.size();
            this->_drawCellAlignedText(curPos, descr.contentWidth(), "/", 1, customStyle,
                                       TableViewCell::TextAlign::LEFT);
            curPos.x += 1;
        }

        if (customStyle) {
            this->_stylist().tableViewTextCell(*this, cell.emphasized());
        }

        this->_drawCellAlignedText(curPos, descr.contentWidth(), filename.c_str(), filename.size(),
                                   customStyle, TableViewCell::TextAlign::LEFT);
    } else {
        std::abort();
    }
}

namespace {

inline Stylist::TableViewCellStyle stylistTvcStyleFromTvcStyle(const TableViewCell::Style style)
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

} // namespace

void TableView::_drawCells(const Index index,
                           const std::vector<std::unique_ptr<TableViewCell>>& cells)
{
    assert(cells.size() == _theColDescrs.size());

    const auto sel = _isSelHighlightEnabledMemb && this->_indexIsSel(index);
    Index x = 0;
    const auto y = _contentYFromIndex(index);

    // set style to clear row initially
    if (sel) {
        this->_stylist().tableViewSel(*this);
    } else {
        this->_stylist().tableViewCell(*this);
    }

    this->_clearRow(y);

    for (Index col = 0; col < cells.size(); ++col) {
        const auto& descr = _theColDescrs[col];
        const auto& cell = *cells[col];
        auto cellStylistTvcStyle = stylistTvcStyleFromTvcStyle(cell.style());

        if (sel) {
            this->_stylist().tableViewSel(*this, cellStylistTvcStyle);
        } else {
            this->_stylist().tableViewCell(*this, cellStylistTvcStyle);
        }

        this->_drawCell({x, y}, _theColDescrs[col], cell,
                        !sel && cell.style() == TableViewCell::Style::NORMAL);
        x += descr.contentWidth();

        if (col == cells.size() - 1) {
            break;
        }

        if (sel) {
            this->_stylist().tableViewSelSep(*this, Stylist::TableViewCellStyle::NORMAL);
        } else {
            this->_stylist().tableViewSep(*this);
        }

        this->_putChar({x, y}, ACS_VLINE);
        x += 1;
    }
}

void TableView::_drawWarningRow(const Index index, const std::string& msg)
{
    const Index x = (this->contentRect().w - msg.size()) / 2;
    const auto y = _contentYFromIndex(index);

    if (this->_indexIsSel(index)) {
        this->_stylist().tableViewSel(*this, Stylist::TableViewCellStyle::WARNING);
    } else {
        this->_stylist().tableViewCell(*this, Stylist::TableViewCellStyle::WARNING);
    }

    this->_clearRow(y);
    this->_moveAndPrint({x, y}, "%s", msg.c_str());
}

void TableView::_redrawRows()
{
    Index index;

    for (index = _theBaseIndex; index < _theBaseIndex + _visibleRowCount; ++index) {
        if (this->_hasIndex(index)) {
            this->_drawRow(index);
        } else {
            break;
        }
    }

    this->_stylist().tableViewSep(*this);

    for (; index < _theBaseIndex + _visibleRowCount; ++index) {
        this->_clearRow(this->_contentYFromIndex(index));
    }

    this->_hasMoreTop(_theBaseIndex > 0);
    this->_hasMoreBottom(this->_hasIndex(_theBaseIndex + _visibleRowCount));
}

void TableView::_resized()
{
    _visibleRowCount = this->contentRect().h - 1;
    _theSelIndex = std::min(_theSelIndex, _theBaseIndex + _visibleRowCount - 1);
}

void TableView::_next(Size count)
{
    assert(count > 0);

    const auto oldSelIndex = _theSelIndex;

    while (true) {
        this->_selIndex(_theSelIndex + count);

        if (_theSelIndex != oldSelIndex) {
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
    if (_theSelIndex == 0) {
        return;
    }

    if (count > _theSelIndex) {
        count = _theSelIndex;
    }

    this->_selIndex(_theSelIndex - count);
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
    const auto idealNewBaseIndex = _theBaseIndex + _visibleRowCount;
    auto effectiveNewBaseIndex = idealNewBaseIndex;

    while (!this->_hasIndex(effectiveNewBaseIndex)) {
        --effectiveNewBaseIndex;
    }

    if (idealNewBaseIndex != effectiveNewBaseIndex) {
        this->_selIndex(effectiveNewBaseIndex);
    } else {
        const auto oldSelIndex = _theSelIndex;

        this->_baseIndex(effectiveNewBaseIndex);
        this->_selIndex(oldSelIndex + _visibleRowCount);
    }
}

void TableView::pageUp()
{
    Index newBaseIndex = 0;
    Index newSelIndex = 0;

    if (_theBaseIndex >= _visibleRowCount) {
        newBaseIndex = _theBaseIndex - _visibleRowCount;
        newSelIndex = _theSelIndex - _visibleRowCount;
    }

    this->_baseIndex(newBaseIndex);
    this->_selIndex(newSelIndex);
}

void TableView::centerSelRow(const bool draw)
{
    if (_theBaseIndex == 0 && this->_maxRowCountFromIndex(0) < _visibleRowCount) {
        // all rows already visible
        return;
    }

    const auto newBaseIndex = static_cast<long long>(_theSelIndex - _visibleRowCount / 2);

    if (newBaseIndex < 0) {
        // row is in the first half
        this->_baseIndex(0);
        return;
    }

    const auto rowCountsFromNewBaseIndex = this->_maxRowCountFromIndex(newBaseIndex);

    if (rowCountsFromNewBaseIndex < _visibleRowCount) {
        // row is in the last half
        this->_baseIndex(static_cast<Index>(newBaseIndex) +
                         rowCountsFromNewBaseIndex - _visibleRowCount, draw);
        return;
    }

    this->_baseIndex(static_cast<Index>(newBaseIndex), draw);
}

void TableView::selectFirst()
{
    this->_selIndex(0);
}

void TableView::_selectLast()
{
}

void TableView::_isSelHighlightEnabled(const bool isEnabled, const bool draw)
{
    if (_isSelHighlightEnabledMemb == isEnabled) {
        return;
    }

    _isSelHighlightEnabledMemb = isEnabled;

    if (draw) {
        this->_redrawRows();
    }
}

} // namespace jacques
