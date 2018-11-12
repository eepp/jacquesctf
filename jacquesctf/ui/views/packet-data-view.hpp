/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_DATA_VIEW_HPP
#define _JACQUES_PACKET_DATA_VIEW_HPP

#include <vector>
#include <list>
#include <algorithm>
#include <unordered_set>

#include "view.hpp"
#include "packet-region.hpp"
#include "inspect-screen.hpp"

namespace jacques {

/*
 * This is the packet inspection view.
 *
 * The view has a current base offset which is the offset of the first
 * visible bit/nibble (top left), if any.
 *
 * The view contains a current vector of character objects. A character
 * is linked to one or more packet regions. A character contains a point
 * and a value to print (e.g., `0`, `1`, `5`, `c`).
 *
 * The view builds character objects from the current packet's packet
 * regions with _setNumericCharsAndAsciiChars(). This method takes the
 * current base offset into account.
 *
 * A character can be linked to more than one packet regions in
 * hexadecimal display mode (when `_isHex` is true). This is because a
 * single nibble can contain up to four individual bits which belong to
 * different packet regions.
 *
 * ASCII characters use the same character class. An ASCII character can
 * have its `isPrintable` property set to false, in which case the view
 * must print an alternative (printable) character with a different
 * style. Hex editors typically use `.` for this (we use `ACS_BULLET`, a
 * centered dot).
 */
class PacketDataView :
    public View
{
public:
    explicit PacketDataView(const Rectangle& rect,
                            const Stylist& stylist, State& state,
                            const InspectScreen::Bookmarks& bookmarks);
    void pageDown();
    void pageUp();
    void centerSelection();
    void isDataInHex(bool isDataInHex);
    void isAsciiVisible(bool isVisible);
    void isPrevNextVisible(bool isVisible);
    void isEventRecordFirstPacketRegionEmphasized(bool isEmphasized);
    void isOffsetInHex(bool isOffsetInHex);
    void isOffsetInBytes(bool isOffsetInBytes);
    void isRowSizePowerOfTwo(bool isPowerOfTwo);

    bool isAsciiVisible() const noexcept
    {
        return _isAsciiVisible;
    }

    bool isPrevNextVisible() const noexcept
    {
        return _isPrevNextVisible;
    }

    bool isEventRecordFirstPacketRegionEmphasized() const noexcept
    {
        return _isEventRecordFirstPacketRegionEmphasized;
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

    const DataSize& rowSize() const noexcept
    {
        return _rowSize;
    }

private:
    struct _Char
    {
        auto packetRegionIt(const Index offsetInPacketBits) const
        {
            return std::find_if(std::begin(packetRegions),
                                std::end(packetRegions),
                                [offsetInPacketBits](const auto& packetRegion) {
                return packetRegion->segment().offsetInPacketBits() == offsetInPacketBits;
            });
        }

        Point pt;
        chtype value;
        std::vector<const PacketRegion *> packetRegions;
        bool isEventRecordFirst = false;
        bool isPrintable = true;
    };

    using _Chars = std::vector<_Char>;

private:
    void _stateChanged(Message msg) override;
    void _redrawContent() override;
    void _resized() override;
    void _drawSeparators() const;
    void _drawOffsets() const;
    void _setTitle();
    void _setCustomStyle(const _Char& ch) const;
    void _drawChar(const _Char& ch) const;
    void _drawUnselectedChar(const _Char& ch) const;
    void _drawAllNumericChars() const;
    void _drawAllAsciiChars() const;
    bool _isCharSelected(const _Char& ch) const;
    void _setDataXAndRowSize();
    void _updateSelection();
    void _setHexChars();
    void _setBinaryChars();
    void _setAsciiChars();
    void _setNumericCharsAndAsciiChars();
    void _setPrevCurNextOffsetInPacketBits();
    void _setBaseAndEndOffsetInPacketBitsFromOffset(Index offsetInPacketBits);

    void _setEndOffsetInPacketBitsFromBaseOffset() noexcept
    {
        const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize().bits();

        _endOffsetInPacketBits = std::min(_baseOffsetInPacketBits + this->_pageSize().bits(),
                                          effectiveTotalSizeBits);
    }

    DataSize _pageSize() const noexcept
    {
        return DataSize {this->contentRect().h * _rowSize.bits()};
    }

    DataSize _halfPageSize() const noexcept
    {
        /*
         * Can't divide `this->_pageSize()` by two here because
         * `this->contentRect().h` could be an odd integer and we want a
         * multiple of `_rowSize.bits()`.
         */
        return DataSize {this->contentRect().h / 2 * _rowSize.bits()};
    }

    Size _charsPerByte() const noexcept
    {
        return _isDataInHex ? 2 : 8;
    }

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
    const InspectScreen::Bookmarks * const _bookmarks;

    // X position of a row's first numeric character
    Index _dataX = 0;

    // X position of a row's first ASCII character
    Index _asciiCharsX = 0;

    // bits/row
    DataSize _rowSize;

    // offset of the top left bit
    Index _baseOffsetInPacketBits = 0;

    // offset of the first invisible bit (last visible bit's offset + 1)
    Index _endOffsetInPacketBits = 0;

    // current numeric characters
    _Chars _chars;

    // current ASCII characters
    _Chars _asciiChars;

    // current packet regions (owned here)
    std::vector<PacketRegion::SPC> _packetRegions;

    boost::optional<Index> _prevOffsetInPacketBits;
    Index _curOffsetInPacketBits = 0;
    boost::optional<Index> _nextOffsetInPacketBits;
    bool _isAsciiVisible = true;
    bool _isPrevNextVisible = true;
    bool _isEventRecordFirstPacketRegionEmphasized = true;
    bool _isDataInHex = true;
    bool _isOffsetInHex = false;
    bool _isOffsetInBytes = true;
    bool _isRowSizePowerOfTwo = true;
};

} // namespace jacques

#endif // _JACQUES_PACKET_DATA_VIEW_HPP
