/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <boost/variant/get.hpp>

#include "help-view.hpp"
#include "stylist.hpp"

namespace jacques {

static constexpr Size keyWidth = 15;

HelpView::HelpView(const Rectangle& rect,
                   const Stylist& stylist) :
    ScrollView {rect, "Help", DecorationStyle::BORDERS, stylist}
{
    this->_buildRows();
    this->_rowCount(_rows.size());
    this->_drawRows();
}

void HelpView::_buildRows()
{
    _rows = {
        _TextRow {"     _____"},
        _TextRow {"    /\\___ \\"},
        _TextRow {"    \\/__/\\ \\     __      ___     __   __  __     __    ____"},
        _TextRow {"       _\\ \\ \\  /'__`\\   /'___\\ /'__`\\/\\ \\/\\ \\  /'__`\\ /',__\\  C"},
        _TextRow {"      /\\ \\_\\ \\/\\ \\L\\.\\_/\\ \\__//\\ \\L\\ \\ \\ \\_\\ \\/\\  __//\\__, `\\  T"},
        _TextRow {"      \\ \\____/\\ \\__/.\\_\\ \\____\\ \\___, \\ \\____/\\ \\____\\/\\____/   F"},
        _TextRow {"       \\/___/  \\/__/\\/_/\\/____/\\/___/\\ \\/___/  \\/____/\\/___/"},
        _TextRow {"                                    \\ \\_\\ " JACQUES_VERSION " \a{eepp.ca}"},
        _TextRow {"                                     \\/_/"},
        _SectionRow {"General keys"},
        _KeyRow {"f", "Go to \"Data stream files\" screen"},
        _KeyRow {"p", "Go to \"Packets\" screen"},
        _KeyRow {"d", "Go to \"Data types\" screen"},
        _KeyRow {"i", "Go to \"Trace info\" screen"},
        _KeyRow {"h, H, ?", "Go to \"Help\" screen"},
        _KeyRow {"q, Esc", "Quit current screen or go to \"Packet inspection\" screen"},
        _KeyRow {"r, Ctrl+l", "Hard refresh screen"},
        _KeyRow {"F10, Q", "Quit program"},
        _EmptyRow {},
        _SectionRow {"\"Packet inspection\" screen"},
        _SubSectionRow {"Presentation"},
        _KeyRow {"e", "Cycle \"Event records\" frame's height"},
        _KeyRow {"Enter", "Show/hide \"Data type\" frame"},
        _KeyRow {"c", "Packet data: center selection"},
        _KeyRow {"v", "Packet data: show/hide previous/next regions"},
        _KeyRow {"w", "Packet data: make bytes/row count a power of two or not"},
        _KeyRow {"a", "Packet data: show/hide ASCII characters"},
        _KeyRow {"o", "Packet data: show offsets in bytes or bits"},
        _KeyRow {"O", "Packet data: show offsets in decimal or hexadecimal"},
        _KeyRow {"x", "Packet data: show bytes in hexadecimal or binary"},
        _KeyRow {"E", "Packet data: emphasize or not event record's first region"},
        _KeyRow {"s", "Event records: Cycle size columns's format"},
        _KeyRow {"t", "Event records: Cycle timestamp column's format"},
        _KeyRow {"#, `", "Show decoding error, if any"},
        _EmptyRow {},
        _SubSectionRow {"Navigation"},
        _KeyRow {"Left", "Go to previous region"},
        _KeyRow {"Right", "Go to next region"},
        _KeyRow {"Up", "Go to region on a preceding row"},
        _KeyRow {"Down", "Go to region on a following row"},
        _KeyRow {"Pg up", "Packet data: move half a page up"},
        _KeyRow {"Pg down", "Packet data: move half a page down"},
        _KeyRow {"Home", "Go to packet's first region"},
        _KeyRow {"End", "Go to packet's last region"},
        _KeyRow {"C", "Go to packet context's first region"},
        _KeyRow {"z", "Go to next region with different parent data type"},
        _KeyRow {"1", "Set/unset region bookmark #1"},
        _KeyRow {"2", "Set/unset region bookmark #2"},
        _KeyRow {"3", "Set/unset region bookmark #3"},
        _KeyRow {"4", "Set/unset region bookmark #4"},
        _KeyRow {"b 1", "Go to region bookmark #1"},
        _KeyRow {"b 2", "Go to region bookmark #2"},
        _KeyRow {"b 3", "Go to region bookmark #3"},
        _KeyRow {"b 4", "Go to region bookmark #4"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _KeyRow {"F5", "Go to previous packet"},
        _KeyRow {"F6", "Go to next packet"},
        _KeyRow {"-", "Go to previous event record"},
        _KeyRow {"+, =, Space", "Go to next event record"},
        _KeyRow {"F7", "Go to the previous 10th event record"},
        _KeyRow {"F8", "Go to the next 10th event record"},
        _KeyRow {"Bksp, 9", "Go back in history"},
        _KeyRow {"0", "Go forward in history"},
        _EmptyRow {},
        _SubSectionRow {"Search/go to"},
        _KeyRow {"/, g", "Search/go to (see syntax below)"},
        _KeyRow {"G", "Live search/go to (see syntax below)"},
        _KeyRow {"P", "Go to packet with index"},
        _KeyRow {":", "Go to offset within data stream"},
        _KeyRow {"$", "Go to offset within packet (bytes)"},
        _KeyRow {"N, *", "Go to event record with timestamp (ns)"},
        _KeyRow {"k", "Go to event record with timestamp (cycles)"},
        _KeyRow {"n", "Repeat previous search"},
        _EmptyRow {},
        _SectionRow {"\"Data stream files\" screen keys"},
        _SubSectionRow {"Presentation"},
        _KeyRow {"s", "Cycle size columns's format"},
        _KeyRow {"t", "Cycle timestamp columns's format"},
        _KeyRow {"c", "Center selected row"},
        _EmptyRow {},
        _SubSectionRow {"Navigation"},
        _KeyRow {"Up", "Select previous row"},
        _KeyRow {"Down", "Select next row"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _EmptyRow {},
        _SubSectionRow {"Action"},
        _KeyRow {"Enter", "Accept selection"},
        _KeyRow {"q", "Discard selection and return to \"Packet inspection\" screen"},
        _EmptyRow {},
        _SectionRow {"\"Packets\" screen keys"},
        _SubSectionRow {"Presentation"},
        _KeyRow {"s", "Cycle size columns's format"},
        _KeyRow {"t", "Cycle timestamp columns's format"},
        _KeyRow {"c", "Center selected row"},
        _EmptyRow {},
        _SubSectionRow {"Navigation"},
        _KeyRow {"Up", "Select previous row"},
        _KeyRow {"Down", "Select next row"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _KeyRow {"F5", "Go to previous packet"},
        _KeyRow {"F6", "Go to next packet"},
        _EmptyRow {},
        _SubSectionRow {"Search/go to"},
        _KeyRow {"/, g", "Search/go to (see syntax below)"},
        _KeyRow {"P", "Go to packet with index"},
        _KeyRow {":", "Go to offset within data stream"},
        _KeyRow {"$", "Go to offset within packet (bytes)"},
        _KeyRow {"N, *", "Go to event record with timestamp (ns)"},
        _KeyRow {"k", "Go to event record with timestamp (cycles)"},
        _KeyRow {"n", "Repeat previous search"},
        _EmptyRow {},
        _SubSectionRow {"Action"},
        _KeyRow {"a", "Analyze all packets"},
        _KeyRow {"Enter", "Accept selection and return to \"Packet inspection\" screen"},
        _KeyRow {"q", "Discard selection and return to \"Packet inspection\" screen"},
        _EmptyRow {},
        _SectionRow {"\"Data types\" screen keys"},
        _SubSectionRow {"Presentation"},
        _KeyRow {"c", "Center selected table row"},
        _KeyRow {"+", "Show/hide tables"},
        _EmptyRow {},
        _SubSectionRow {"Navigation"},
        _KeyRow {"Up", "Select previous row/go up one line"},
        _KeyRow {"Down", "Select next row/go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"Tab, Enter", "Focus next frame"},
        _KeyRow {"Left", "Focus left frame"},
        _KeyRow {"Right", "Focus right frame"},
        _EmptyRow {},
        _SubSectionRow {"Search"},
        _KeyRow {"/, g", "Search event record type (name or ID, see syntax below)"},
        _KeyRow {"n", "Repeat previous search"},
        _EmptyRow {},
        _SubSectionRow {"Action"},
        _KeyRow {"q", "Return to \"Packet inspection\" screen"},
        _EmptyRow {},
        _SectionRow {"\"Trace info\" screen keys"},
        _KeyRow {"Up", "Select previous row/go up one line"},
        _KeyRow {"Down", "Select next row/go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"q", "Return to \"Packet inspection\" screen"},
        _EmptyRow {},
        _SectionRow {"\"Help\" screen keys"},
        _KeyRow {"Up", "Go up one line"},
        _KeyRow {"Down", "Go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"q", "Return to previous screen"},
        _EmptyRow {},
        _SectionRow {"Search box keys"},
        _KeyRow {"Ctrl+w", "Clear input"},
        _KeyRow {"Enter", "Search if input is not empty, else cancel"},
        _KeyRow {"Ctrl+d", "Cancel"},
        _EmptyRow {},
        _SectionRow {"Search syntax"},
        _TextRow {"X is a constant integer which you can write in decimal, hexadecimal"},
        _TextRow {"(`0x` prefix), or octal (`0` prefix). Commas (`,`) are ignored in"},
        _TextRow {"decimal constant integers (for example, `192,283,038`)."},
        _EmptyRow {},
        _SearchSyntaxRow {"Packet at index X within data stream file", "#X"},
        _SearchSyntaxRow {"Packet at relative index X within data stream file", "+#X  -#X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Packet with sequence number X", "##X"},
        _SearchSyntaxRow {"Packet with relative sequence number X", "+##X  -##X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Event record at index X within packet", "@X"},
        _SearchSyntaxRow {"Event record at relative index X within packet", "+@X  -@X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Offset X (bits) within packet", "X"},
        _SearchSyntaxRow {"Relative offset X (bits) within packet", "+X  -X"},
        _SearchSyntaxRow {"Offset X (bytes) within packet", "$X"},
        _SearchSyntaxRow {"Relative offset X (bytes) within packet", "+$X  -$X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Offset X (bits) within data stream file", ":X"},
        _SearchSyntaxRow {"Relative offset X (bits) within data stream file", "+:X  -:X"},
        _SearchSyntaxRow {"Offset X (bytes) within data stream file", ":$X"},
        _SearchSyntaxRow {"Relative offset X (bytes) within data stream file", "+:$X  -:$X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Event record with timestamp X (ns from origin)", "*X  *-X"},
        _SearchSyntaxRow {"Event record with relative timestamp X (ns from origin)", "+*X  -*X"},
        _SearchSyntaxRow {"Event record with timestamp X (cycles)", "**X"},
        _SearchSyntaxRow {"Event record with relative timestamp X (cycles)", "+**X  -**X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Next event record with type name NAME", "/NAME"},
        _SearchSyntaxRow {"Next event record with type ID X", "%X"},
    };

    _ssRowFmtPos = 0;
    _longestRowWidth = 0;

    for (const auto& row : _rows) {
        if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            _ssRowFmtPos = std::max(_ssRowFmtPos,
                                    static_cast<Size>(ssRow->descr.size() + 4));
        }
    }

    for (const auto& row : _rows) {
        if (auto sectRow = boost::get<_SectionRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(sectRow->title.size()));
        } else if (auto sectRow = boost::get<_SubSectionRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(sectRow->title.size()));
        } else if (auto keyRow = boost::get<_KeyRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(keyWidth + keyRow->descr.size() + 2));
        } else if (auto textRow = boost::get<_TextRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(textRow->line.size()));
        } else if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(ssRow->format.size() + _ssRowFmtPos));
        }
    }
}

void HelpView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();

    const Index startX = (this->contentRect().w - _longestRowWidth) / 2;

    for (Index rowNum = 0; rowNum < this->contentRect().h; ++rowNum) {
        const auto y = rowNum;
        const auto rowIdx = this->_index() + rowNum;

        if (rowIdx >= this->_rowCount()) {
            break;
        }

        const auto& row = _rows[rowIdx];

        if (auto sectRow = boost::get<_SectionRow>(&row)) {
            this->_stylist().helpViewSection(*this);
            this->_moveAndPrint({startX, y}, "%s", sectRow->title.c_str());
        } else if (auto sectRow = boost::get<_SubSectionRow>(&row)) {
            this->_stylist().helpViewSubSection(*this);
            this->_moveAndPrint({startX + 2, y}, "%s", sectRow->title.c_str());
        } else if (auto keyRow = boost::get<_KeyRow>(&row)) {
            this->_moveCursor({startX + 2, y});

            for (const auto& ch : keyRow->key) {
                if (ch == ',') {
                    this->_stylist().std(*this);
                } else {
                    this->_stylist().helpViewKey(*this);
                }

                this->_appendChar(ch);
            }

            this->_stylist().std(*this);
            this->_moveAndPrint({startX + keyWidth, y}, "%s",
                                keyRow->descr.c_str());
        } else if (auto textRow = boost::get<_TextRow>(&row)) {
            this->_stylist().std(*this);
            this->_moveCursor({startX + 2, y});
            this->_stylist().std(*this, textRow->bold);

            for (const auto& ch : textRow->line) {
                if (ch == '\a') {
                    this->_stylist().stdDim(*this);
                    continue;
                }

                this->_appendChar(ch);
            }
        } else if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            this->_stylist().std(*this);
            this->_moveCursor({startX + 2, y});

            for (const auto& ch : ssRow->descr) {
                if (ch == 'X') {
                    this->_stylist().helpViewKey(*this);
                } else {
                    this->_stylist().std(*this);
                }

                this->_appendChar(ch);
            }

            this->_moveCursor({startX + _ssRowFmtPos, y});
            this->_stylist().helpViewKey(*this);

            for (const auto& ch : ssRow->format) {
                if (ch == 'X') {
                    this->_stylist().helpViewKey(*this);
                } else {
                    this->_stylist().std(*this);
                }

                this->_appendChar(ch);
            }
        }
    }
}

} // namespace jacques
