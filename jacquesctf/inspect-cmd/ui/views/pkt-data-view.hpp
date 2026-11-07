/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DATA_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DATA_VIEW_HPP

#include <algorithm>
#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include "aliases.hpp"
#include "view.hpp"
#include "data/pkt-region.hpp"

namespace jacques {

/*
 * This is the packet inspection view.
 *
 * The view has a current base offset which is the offset of the first
 * visible bit/nibble (top left), if any.
 *
 * The view contains a current vector of character objects. A character
 * is linked to one or more packet regions. A character contains a point
 * and a value to print (for example, `0`, `1`, `5`, `c`).
 *
 * The view builds character objects from the regions of the current
 * packet with _setNumericCharsAndAsciiChars(). This method takes the
 * current base offset into account.
 *
 * A character can be linked to more than one packet regions in
 * hexadecimal display mode (when `_isHex` is true). This is because a
 * single nibble can contain up to four individual bits which may belong
 * to different packet regions.
 *
 * ASCII characters use the same character class. An ASCII character can
 * have its `isPrintable` property set to false, in which case the view
 * must print an alternative (printable) character with a different
 * style. Hex editors typically use `.` for this (we use `glyphs::BULLET`, a
 * centered dot).
 */
class PktDataView final :
    public View,
    private ViewInspectCmdStateObserver
{
public:
    // bookmarked offsets within a packet (bits)
    using PktBookmarks = std::array<std::optional<Index>, 4>;

    // packet index within data stream file to packet bookmarks
    using DsFileBookmarks = std::unordered_map<Index, PktBookmarks>;

    // data stream file index to data stream file bookmarks
    using Bookmarks = std::unordered_map<Index, DsFileBookmarks>;

public:
    explicit PktDataView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                         const Bookmarks& bookmarks);
    void pageDown();
    void pageUp();
    void centerSel();
    void isDataInHex(bool isDataInHex);
    void isAsciiVisible(bool isVisible);
    void isPrevNextVisible(bool isVisible);
    void isErFirstPktRegionEmphasized(bool isEmphasized);
    void isOffsetInHex(bool isOffsetInHex);
    void isOffsetInBytes(bool isOffsetInBytes);
    void isRowSizePowerOfTwo(bool isPowerOfTwo);
    void isOffsetInPkt(bool isOffsetInPkt);

    /*
     * Call this when the bookmarks which this view was built with
     * change.
     */
    void bookmarksChanged() noexcept
    {
        this->_invalidate();
    }

    bool isAsciiVisible() const noexcept
    {
        return _isAsciiVisible;
    }

    bool isPrevNextVisible() const noexcept
    {
        return _isPrevNextVisible;
    }

    bool isErFirstPktRegionEmphasized() const noexcept
    {
        return _isErFirstPktRegionEmphasized;
    }

    bool isDataInHex() const noexcept
    {
        return _isDataInHex;
    }

    bool isOffsetInHex() const noexcept
    {
        return _isOffsetInHex;
    }

    bool isOffsetInBytes() const noexcept
    {
        return _isOffsetInBytes;
    }

    bool isRowSizePowerOfTwo() const noexcept
    {
        return _isRowSizePowerOfTwo;
    }

    bool isOffsetInPkt() const noexcept
    {
        return _isOffsetInPkt;
    }

    const DataLen& rowSize() const noexcept
    {
        return _rowSize;
    }

private:
    struct _Char final
    {
        explicit _Char() = default;
        _Char(const _Char&) = default;
        _Char& operator=(const _Char&) = default;
        _Char(_Char&&) = default;
        _Char& operator=(_Char&&) = default;

        auto pktRegionIt(const Index offsetInPktBits) const
        {
            return std::find_if(pktRegions.begin(), pktRegions.end(),
                                [offsetInPktBits](const auto& pktRegion) {
                return pktRegion->segment().offsetInPktBits() == offsetInPktBits;
            });
        }

        Point pt;
        char val;
        std::vector<const PktRegion *> pktRegions;
        bool isErFirst = false;
        bool isPrintable = true;
    };

    using _Chars = std::vector<_Char>;

private:
    void _appStateChanged(Message msg) override;
    void _redrawContent() override;
    void _resized() override;
    void _drawSeparators() const;
    void _drawOffsets() const;
    void _setTitle();
    void _setCustomStyle(const _Char& ch) const;
    void _drawChar(const _Char& ch) const;
    void _writeCharVal(const _Char& ch) const;
    void _drawUnselChar(const _Char& ch) const;
    void _drawAllNumericChars() const;
    void _drawAllAsciiChars() const;
    void _setDataXAndRowSize();
    void _updateSel();
    void _setHexChars();
    void _setBinaryChars();
    void _setAsciiChars();
    void _setNumericCharsAndAsciiChars();
    void _setPrevCurNextOffsetInPktBits();
    void _setBaseAndEndOffsetInPktBitsFromOffset(Index offsetInPktBits);

    void _setEndOffsetInPktBitsFromBaseOffset() noexcept
    {
        const auto effectiveTotalLenBits = this->_appState().activePktState().pkt().indexEntry().effectiveTotalLen().bits();

        _charsAreStale = true;
        _endOffsetInPktBits = std::min(_baseOffsetInPktBits + this->_pageLen().bits(),
                                       effectiveTotalLenBits);
    }

    DataLen _pageLen() const noexcept
    {
        return DataLen {this->contentRect().h * _rowSize.bits()};
    }

    DataLen _halfPageLen() const noexcept
    {
        /*
         * Can't divide `this->_pageLen()` by two here because
         * `this->contentRect().h` could be an odd integer and we want a
         * multiple of `_rowSize.bits()`.
         */
        return DataLen {this->contentRect().h / 2 * _rowSize.bits()};
    }

    Size _charsPerByte() const noexcept
    {
        return _isDataInHex ? 2 : 8;
    }

private:
    const Bookmarks *_bookmarks;

    // X position of the first numeric character of a row
    Index _dataX = 0;

    // X position of the first ASCII character of a row
    Index _asciiCharsX = 0;

    // bits/row
    DataLen _rowSize;

    // offset of the top left bit
    Index _baseOffsetInPktBits = 0;

    // offset of the first invisible bit (last visible bit offset + 1)
    Index _endOffsetInPktBits = 0;

    // current numeric characters
    _Chars _chars;

    // current ASCII characters
    _Chars _asciiChars;

    // current packet regions (owned here)
    std::vector<PktRegion::SPC> _pktRegions;

    /*
     * True if `_chars`, `_asciiChars`, and `_pktRegions` don't match
     * the current page and format anymore (see _redrawContent()).
     */
    bool _charsAreStale = true;

    std::optional<Index> _prevOffsetInPktBits;
    Index _curOffsetInPktBits = 0;
    std::optional<Index> _nextOffsetInPktBits;
    bool _isAsciiVisible = true;
    bool _isPrevNextVisible = true;
    bool _isErFirstPktRegionEmphasized = true;
    bool _isDataInHex = true;
    bool _isOffsetInHex = false;
    bool _isOffsetInBytes = true;
    bool _isRowSizePowerOfTwo = true;
    bool _isOffsetInPkt = true;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DATA_VIEW_HPP
