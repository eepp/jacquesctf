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
 * The view contains a current vector of zones. A zone is a vector of
 * zone characters, and it is also linked to a packet region. A zone
 * character is a point and a character (e.g., `0`, `1`, `5`, `c`).
 * Therefore a zone contains which exact characters to draw on the view
 * and their value.
 *
 * The view builds zones from the current packet's packet regions with
 * _setZonesAndAsciiChars(). This method takes the current base offset
 * into account.
 *
 * The order of the current zones is the same as their underlying packet
 * regions. However, because we're showing bytes with the positional
 * notation (where the byte's LSB is the right-most bit), little endian
 * packet regions can lead to holes within zones. For example, in
 * binary:
 *
 *     Bits: 11010010 00101001 10010010 11110101
 *     Zone: 32221111 33333333 44433333 55544444
 *
 * The first zone is on the right of the first byte, and the following
 * zone is on its left. The third zone is on the right of the first
 * byte, on all the second byte, and on the right of the third byte.
 *
 * Of course, if we were to make the left-most byte's bit the LSB, the
 * zones would be naturally ordered:
 *
 *     Bits: 01001011 10010100 01001001 10101111
 *     Zone: 11112223 33333333 33333444 44444555
 *
 * However:
 *
 * 1. Most software developers and computer engineers are used to
 *    the LSB being the byte's right-most bit.
 * 2. In a CTF data stream, the byte order can change from one byte to
 *    the other: in this case, big endian zones would look odd.
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
    void isHex(bool isHex);
    void isAsciiVisible(bool isVisible);
    void isEventRecordFirstPacketRegionEmphasized(bool isEmphasized);

    bool isAsciiVisible() const noexcept
    {
        return _isAsciiVisible;
    }

    bool isEventRecordFirstPacketRegionEmphasized() const noexcept
    {
        return _isEventRecordFirstPacketRegionEmphasized;
    }

    bool isHex() const noexcept
    {
        return _isHex;
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
        std::vector<PacketRegion::SPC> packetRegions;
        bool isEventRecordFirst = false;
    };

    using _Chars = std::vector<_Char>;

    struct _AsciiChar
    {
        Index endOffsetInPacketBits() const noexcept
        {
            return offsetInPacketBits + 8;
        }

        Index offsetInPacketBits;
        Point pt;
        char ch;
    };

    using _AsciiChars = std::vector<_AsciiChar>;

private:
    void _stateChanged(const Message& msg) override;
    void _redrawContent() override;
    void _resized() override;
    void _drawOffsets() const;
    void _setCustomStyle(const _Char& ch) const;
    void _drawChar(const _Char& ch) const;
    void _drawUnselectedChar(const _Char& ch) const;
    void _drawAllNumericChars() const;
    void _drawAllAsciiChars() const;
    bool _isCharSelected(const _Char& ch) const;
    void _setDataXAndRowSize();
    void _updateSelection();
    void _setHexChars(std::vector<PacketRegion::SPC>& packetRegions);
    void _setBinaryChars(std::vector<PacketRegion::SPC>& packetRegions);
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
        return _isHex ? 2 : 8;
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
    _AsciiChars _asciiChars;

    boost::optional<Index> _prevOffsetInPacketBits;
    Index _curOffsetInPacketBits = 0;
    boost::optional<Index> _nextOffsetInPacketBits;
    bool _isAsciiVisible = true;
    bool _isEventRecordFirstPacketRegionEmphasized = true;
    bool _isHex = true;
};

} // namespace jacques

#endif // _JACQUES_PACKET_DATA_VIEW_HPP
