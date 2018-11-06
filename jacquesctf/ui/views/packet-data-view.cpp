/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cinttypes>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

#include "stylist.hpp"
#include "packet-data-view.hpp"
#include "utils.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
#include "cur-offset-in-packet-changed-message.hpp"
#include "content-packet-region.hpp"
#include "padding-packet-region.hpp"
#include "error-packet-region.hpp"
#include "inspect-screen.hpp"

namespace jacques {

PacketDataView::PacketDataView(const Rectangle& rect,
                               const Stylist& stylist, State& state,
                               const InspectScreen::Bookmarks& bookmarks) :
    View {rect, "Packet data", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this},
    _bookmarks {&bookmarks}
{
}

void PacketDataView::_resized()
{
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setPrevCurNextOffsetInPacketBits();
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_redrawContent();
}

void PacketDataView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg) ||
            dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        if (_state->hasActivePacketState()) {
            this->_setDataXAndRowSize();
            this->_setPrevCurNextOffsetInPacketBits();
            this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
        }

        this->_redrawContent();
    } else if (dynamic_cast<const CurOffsetInPacketChangedMessage *>(&msg)) {
        this->_updateSelection();
    }
}

void PacketDataView::_setBaseAndEndOffsetInPacketBitsFromOffset(const Index offsetInPacketBits)
{
    assert(_state->hasActivePacketState());
    assert(offsetInPacketBits <
           _state->activePacketState().packet().indexEntry().effectiveTotalSize());

    const auto rowOffsetInPacketBits = offsetInPacketBits -
                                       (offsetInPacketBits % _rowSize.bits());
    const auto halfPageSize = this->_halfPageSize();

    if (rowOffsetInPacketBits < halfPageSize) {
        _baseOffsetInPacketBits = 0;
    } else {
        _baseOffsetInPacketBits = rowOffsetInPacketBits - halfPageSize.bits();
    }

    this->_setEndOffsetInPacketBitsFromBaseOffset();
}

void PacketDataView::_setPrevCurNextOffsetInPacketBits()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    // current
    _curOffsetInPacketBits = _state->curOffsetInPacketBits();

    const auto curPacketRegion = _state->currentPacketRegion();
    auto& packet = _state->activePacketState().packet();

    assert(curPacketRegion);

    // previous
    const auto prevPacketRegion = packet.previousPacketRegion(*curPacketRegion);

    if (prevPacketRegion) {
        _prevOffsetInPacketBits = prevPacketRegion->segment().offsetInPacketBits();
    } else {
        _prevOffsetInPacketBits = boost::none;
    }

    // next
    if (curPacketRegion->segment().endOffsetInPacketBits() <
            packet.indexEntry().effectiveTotalSize()) {
        _nextOffsetInPacketBits = curPacketRegion->segment().endOffsetInPacketBits();
    } else {
        _nextOffsetInPacketBits = boost::none;
    }
}

bool PacketDataView::_isCharSelected(const _Char& ch) const
{
    const auto selectedPacketRegionIt = std::find_if(std::begin(ch.packetRegions),
                                                     std::end(ch.packetRegions),
                                                     [this](const auto& packetRegion) {
        return packetRegion->segment().offsetInPacketBits() == _curOffsetInPacketBits;
    });

    if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
        return true;
    }

    if (_isHex) {
        return false;
    }

    const auto chSingleOffsetInPacketBits = ch.packetRegions.front()->segment().offsetInPacketBits();

    return (_prevOffsetInPacketBits &&
            chSingleOffsetInPacketBits == *_prevOffsetInPacketBits) ||
           (_nextOffsetInPacketBits &&
            chSingleOffsetInPacketBits == *_nextOffsetInPacketBits);
}

void PacketDataView::_updateSelection()
{
    assert(_state->hasActivePacketState());

    if (_state->curOffsetInPacketBits() < _baseOffsetInPacketBits ||
            _state->curOffsetInPacketBits() >= _endOffsetInPacketBits ||
            _chars.empty()) {
        // outside the current character: reset base offset
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_state->curOffsetInPacketBits());
        this->_setPrevCurNextOffsetInPacketBits();
        this->_redrawContent();
        return;
    }

    // "erase" currently selected characters
    // TODO: do not search linearly
    for (const auto& ch : _chars) {
        if (this->_isCharSelected(ch)) {
            this->_drawUnselectedChar(ch);
        }
    }

    // draw new selected characters
    // TODO: do not search linearly
    this->_setPrevCurNextOffsetInPacketBits();

    for (const auto& ch : _chars) {
        if (this->_isCharSelected(ch)) {
            this->_drawChar(ch);
        }
    }

    // update offset
    this->_drawOffsets();

    // update ASCII chars
    // TODO: unselect and select other like we're doing it for the num chars
    this->_drawAllAsciiChars();
}

void PacketDataView::_setDataXAndRowSize()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    // we can compute log16, or we can do that:
    std::array<char, 16> buf;

    std::sprintf(buf.data(), "%" PRIx64,
                 static_cast<std::uint64_t>(_state->activePacketState().packetIndexEntry().effectiveTotalSize().bits()));

    const auto offsetWidth = std::strlen(buf.data());

    Size bytesPerRow = 1;

    while (true) {
        // bytes + spaces between them
        auto dataWidth = bytesPerRow * (this->_charsPerByte() + 1) - 1;

        if (_isAsciiVisible) {
            // ASCII characters + space before them
            dataWidth += bytesPerRow + 1;
        }

        if (dataWidth + offsetWidth + 1 > this->contentRect().w) {
            bytesPerRow /= 2;
            break;
        }

        bytesPerRow *= 2;
    }

    _dataX = offsetWidth + 1;
    _asciiCharsX = _dataX + bytesPerRow * (this->_charsPerByte() + 1);
    _rowSize = DataSize::fromBytes(bytesPerRow);
}

void PacketDataView::_drawOffsets() const
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    assert(_dataX > 0 && _rowSize > 0);

    const auto& packetTotalSize = _state->activePacketState().packetIndexEntry().effectiveTotalSize();
    const auto maxOffsetWidth = _dataX - 1;
    auto offsetInPacketBits = _baseOffsetInPacketBits;

    for (Index y = 0; y < this->contentRect().h; ++y) {
        if (offsetInPacketBits >= packetTotalSize) {
            // clear
            for (Index x = 0; x < _dataX - 1; ++x) {
                this->_putChar({x, y}, ' ');
            }
        } else {
            std::array<char, 16> buf;

            std::sprintf(buf.data(), "%" PRIx64,
                         static_cast<std::uint64_t>(offsetInPacketBits));

            const auto offsetWidth = std::strlen(buf.data());

            this->_moveCursor({0, y});

            const auto selected = _curOffsetInPacketBits >= offsetInPacketBits &&
                                  _curOffsetInPacketBits < offsetInPacketBits + _rowSize;

            this->_stylist().packetDataViewOffset(*this, selected);

            for (Index x = 0; x < maxOffsetWidth - offsetWidth; ++x) {
                this->_appendChar(' ');
            }

            this->_print("%s", buf.data());
            offsetInPacketBits += _rowSize.bits();
        }
    }
}

void PacketDataView::_setBookmarkStyle(const _Char& ch) const
{
    assert(!_isHex);

    const auto it = _bookmarks->find(_state->activeDataStreamFileStateIndex());
    const InspectScreen::PacketBookmarks *bookmarks = nullptr;

    if (it != std::end(*_bookmarks)) {
        const auto& dataStreamFileBookmarks = it->second;
        const auto pIt = dataStreamFileBookmarks.find(_state->activeDataStreamFileState().activePacketStateIndex());

        if (pIt != std::end(dataStreamFileBookmarks)) {
            bookmarks = &pIt->second;
        }
    }

    bool bookmark = false;
    const auto& singlePacketRegion = *ch.packetRegions.front();

    if (bookmarks) {
        for (Index id = 0; id < bookmarks->size(); ++id) {
            if (bookmarks->at(id) &&
                    *(bookmarks->at(id)) == singlePacketRegion.segment().offsetInPacketBits()) {
                this->_stylist().packetDataViewBookmark(*this, id);
                bookmark = true;
                break;
            }
        }
    }

    if (!bookmark) {
        if (ch.isEventRecordFirst && _isEventRecordFirstPacketRegionEmphasized) {
            this->_stylist().packetDataViewEventRecordFirstPacketRegion(*this);
        } else if (const auto region = dynamic_cast<const ContentPacketRegion *>(&singlePacketRegion)) {
            this->_stylist().std(*this);
        } else if (const auto region = dynamic_cast<const PaddingPacketRegion *>(&singlePacketRegion)) {
            this->_stylist().packetDataViewPadding(*this);
        } else if (const auto region = dynamic_cast<const ErrorPacketRegion *>(&singlePacketRegion)) {
            this->_stylist().error(*this);
        } else {
            std::abort();
        }
    }
}

void PacketDataView::_drawUnselectedChar(const _Char& ch) const
{
    if (_isHex) {
        this->_stylist().std(*this);
    } else {
        this->_setBookmarkStyle(ch);
    }

    this->_putChar(ch.pt, ch.value);
}

void PacketDataView::_drawChar(const _Char& ch) const
{
    const auto& singlePacketRegionOffsetBits = ch.packetRegions.front()->segment().offsetInPacketBits();
    const auto selectedPacketRegionIt = std::find_if(std::begin(ch.packetRegions),
                                                     std::end(ch.packetRegions),
                                                     [this](const auto& packetRegion) {
        return packetRegion->segment().offsetInPacketBits() == _curOffsetInPacketBits;
    });

    if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
        if (ch.packetRegions.size() == 1) {
            this->_stylist().packetDataViewSelection(*this,
                                                     Stylist::PacketDataViewSelectionType::CURRENT);
        } else {
            this->_stylist().packetDataViewAuxSelection(*this,
                                                        Stylist::PacketDataViewSelectionType::CURRENT);
        }
    } else if (!_isHex && _prevOffsetInPacketBits &&
            singlePacketRegionOffsetBits == *_prevOffsetInPacketBits) {
        this->_stylist().packetDataViewSelection(*this,
                                                 Stylist::PacketDataViewSelectionType::PREVIOUS);
    } else if (!_isHex && _nextOffsetInPacketBits &&
            singlePacketRegionOffsetBits == *_nextOffsetInPacketBits) {
        this->_stylist().packetDataViewSelection(*this,
                                                 Stylist::PacketDataViewSelectionType::NEXT);
    } else {
        this->_drawUnselectedChar(ch);
    }

    this->_putChar(ch.pt, ch.value);
}

void PacketDataView::_drawAllNumericChars() const
{
    for (const auto& ch : _chars) {
        this->_drawChar(ch);
    }
}

void PacketDataView::_drawAllAsciiChars() const
{
    if (!_isAsciiVisible) {
        return;
    }

    // clear
    this->_stylist().std(*this);

    for (Index i = 0; i < _rowSize.bytes(); ++i) {
        this->_putChar({_asciiCharsX + i, i / _rowSize.bytes()}, ' ');
    }

    const auto curPacketRegionSeg = _state->currentPacketRegion()->segment();

    for (const auto& asciiChar : _asciiChars) {
        const auto intersectLower = std::max(curPacketRegionSeg.offsetInPacketBits(),
                                             asciiChar.offsetInPacketBits);
        const auto intersectUpper = std::min(curPacketRegionSeg.endOffsetInPacketBits(),
                                             asciiChar.endOffsetInPacketBits());
        const auto intersects = intersectUpper > intersectLower;

        if (intersects) {
            this->_stylist().packetDataViewAuxSelection(*this,
                                                        Stylist::PacketDataViewSelectionType::CURRENT);
        } else {
            if (std::isprint(asciiChar.ch)) {
                this->_stylist().std(*this);
            } else {
                this->_stylist().stdDim(*this);
            }
        }

        const chtype ch = std::isprint(asciiChar.ch) ? asciiChar.ch : ACS_BULLET;

        this->_putChar(asciiChar.pt, ch);
    }
}

void PacketDataView::_redrawContent()
{
    this->_clearContent();

    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_drawOffsets();
    this->_setNumericCharsAndAsciiChars();
    this->_drawAllNumericChars();
    this->_drawAllAsciiChars();
    this->_hasMoreTop(_baseOffsetInPacketBits != 0);

    const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize();

    this->_hasMoreBottom(_endOffsetInPacketBits < effectiveTotalSizeBits);
}

static inline chtype charFromNibble(const std::uint8_t nibble)
{
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return 'a' + nibble - 10;
    }
}

void PacketDataView::_setHexChars(std::vector<PacketRegion::SPC>& packetRegions)
{
    /*
     * The strategy here is to create all the nibble characters first,
     * and then iterate the packet regions and find the already-created
     * character to append a packet region (if not already appended).
     */
    const auto data = _state->activePacketState().packet().data(_baseOffsetInPacketBits / 8);

    for (auto offsetInPacketBits = _baseOffsetInPacketBits;
            offsetInPacketBits < _endOffsetInPacketBits;
            offsetInPacketBits += 8) {
        const auto byte = data[(offsetInPacketBits - _baseOffsetInPacketBits) / 8];
        const auto y = (offsetInPacketBits - _baseOffsetInPacketBits) /
                       _rowSize.bits();
        const auto byteIndexInRow = (offsetInPacketBits % _rowSize.bits()) / 8;

        // high nibble
        {
            _Char ch;

            ch.pt.y = y;
            ch.pt.x = _dataX + byteIndexInRow * 3;

            const auto nibble = (byte >> 4) & 0xf;

            ch.value = charFromNibble(nibble);
            _chars.push_back(std::move(ch));
        }

        // low nibble
        {
            _Char ch;

            ch.pt.y = y;
            ch.pt.x = _dataX + byteIndexInRow * 3 + 1;

            const auto nibble = byte & 0xf;

            ch.value = charFromNibble(nibble);
            _chars.push_back(std::move(ch));
        }
    }

    const EventRecord *curEventRecord = nullptr;

    for (auto& packetRegion : packetRegions) {
        const auto bitArray = _state->activePacketState().packet().bitArray(*packetRegion);
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();

        bool isEventRecordFirst = false;

        if (packetRegion->scope() && packetRegion->scope()->eventRecord() &&
                packetRegion->scope()->eventRecord() != curEventRecord) {
            curEventRecord = packetRegion->scope()->eventRecord();
            isEventRecordFirst = true;
        }

        /*
         * Iterate the whole bit array. This is just simpler: one out of
         * four bits won't alter a character.
         */
        for (Index indexInBitArray = 0;
                indexInBitArray < packetRegion->segment().size();
                ++indexInBitArray) {
            const auto bitOffsetInPacket = firstBitOffsetInPacket +
                                           indexInBitArray;

            if (bitOffsetInPacket < _baseOffsetInPacketBits ||
                    bitOffsetInPacket >= _endOffsetInPacketBits) {
                // not visible
                continue;
            }

            // times two because `_chars` contains nibbles, not bytes
            auto charIndex = ((bitOffsetInPacket / 8) -
                              (_baseOffsetInPacketBits / 8)) * 2;
            const auto bitLoc = bitArray.bitLocation(indexInBitArray);

            if (bitLoc.bitIndexInByte() < 4) {
                // low nibble
                charIndex += 1;
            }

            assert(charIndex < _chars.size());

            // get existing character (created above)
            auto& ch = _chars[charIndex];

            ch.isEventRecordFirst = isEventRecordFirst;

            if (ch.packetRegions.empty() ||
                    ch.packetRegions.front().get() != packetRegion.get()) {
                // associate character to packet region
                ch.packetRegions.push_back(packetRegion);
            }
        }
    }
}

void PacketDataView::_setBinaryChars(std::vector<PacketRegion::SPC>& packetRegions)
{
    const EventRecord *curEventRecord = nullptr;

    for (auto& packetRegion : packetRegions) {
        const auto bitArray = _state->activePacketState().packet().bitArray(*packetRegion);
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();
        bool isEventRecordFirst = false;

        if (packetRegion->scope() && packetRegion->scope()->eventRecord() &&
                packetRegion->scope()->eventRecord() != curEventRecord) {
            curEventRecord = packetRegion->scope()->eventRecord();
            isEventRecordFirst = true;
        }

        for (Index indexInBitArray = 0;
                indexInBitArray < packetRegion->segment().size();
                ++indexInBitArray) {
            const auto bitOffsetInPacket = firstBitOffsetInPacket +
                                           indexInBitArray;

            if (bitOffsetInPacket < _baseOffsetInPacketBits ||
                    bitOffsetInPacket >= _endOffsetInPacketBits) {
                // not visible
                continue;
            }

            _Char ch;
            const auto bitLoc = bitArray.bitLocation(indexInBitArray);

            ch.value = '0' + bitArray[bitLoc];
            ch.pt.y = (bitOffsetInPacket - _baseOffsetInPacketBits) /
                      _rowSize.bits();

            const auto byteIndex = (bitOffsetInPacket % _rowSize.bits()) / 8;

            /*
             * 28000 00101101 11010010 11101010 00010010
             *       ^ _dataX [6]      ^ + byteIndex * 9 [+ 2 * 9]
             *                              ^ + 7 - bitLoc.bitIndexInByte [+ 7 - 2]
             */
            ch.pt.x = _dataX + byteIndex * 9 + 7 - bitLoc.bitIndexInByte();
            ch.packetRegions.push_back(packetRegion);
            _chars.push_back(std::move(ch));
        }
    }
}

void PacketDataView::_setNumericCharsAndAsciiChars()
{
    if (_baseOffsetInPacketBits == _endOffsetInPacketBits) {
        return;
    }

    assert(_baseOffsetInPacketBits < _endOffsetInPacketBits);
    assert(_state->hasActivePacketState());

    // set numeric characters
    std::vector<PacketRegion::SPC> packetRegions;
    auto& packet = _state->activePacketState().packet();

    /*
     * We'll go one packet region before to detect an initial
     * event record change.
     */
    const auto& basePacketRegion = packet.packetRegionAtOffsetInPacketBits(_baseOffsetInPacketBits);
    auto startingPacketRegion = packet.previousPacketRegion(basePacketRegion);

    if (!startingPacketRegion) {
        startingPacketRegion = &basePacketRegion;
    }

    packet.appendPacketRegions(packetRegions,
                               startingPacketRegion->segment().offsetInPacketBits(),
                               _endOffsetInPacketBits);
    assert(!packetRegions.empty());
    _chars.clear();

    if (_isHex) {
        this->_setHexChars(packetRegions);
    } else {
        this->_setBinaryChars(packetRegions);
    }

    // set ASCII chars
    _asciiChars.clear();
    assert((_baseOffsetInPacketBits & 7) == 0);

    for (auto offsetInPacketBits = _baseOffsetInPacketBits;
            offsetInPacketBits < _endOffsetInPacketBits; offsetInPacketBits += 8) {
        Point pt;

        pt.y = (offsetInPacketBits - _baseOffsetInPacketBits) / _rowSize.bits();
        pt.x = _asciiCharsX + (offsetInPacketBits % _rowSize.bits()) / 8;
        _asciiChars.push_back(_AsciiChar {
            offsetInPacketBits, pt,
            static_cast<char>(*packet.data(offsetInPacketBits / 8))
        });
    }
}

void PacketDataView::pageDown()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize().bits();

    _baseOffsetInPacketBits += this->_halfPageSize().bits();

    if (_baseOffsetInPacketBits >= effectiveTotalSizeBits) {
        const auto endRowOffsetInPacketBits = effectiveTotalSizeBits -
                                              (effectiveTotalSizeBits % _rowSize.bits());

        if (endRowOffsetInPacketBits == 0) {
            _baseOffsetInPacketBits = 0;
        } else {
            _baseOffsetInPacketBits = endRowOffsetInPacketBits - _rowSize.bits();
        }
    }

    this->_setEndOffsetInPacketBitsFromBaseOffset();
    this->_redrawContent();
}

void PacketDataView::pageUp()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    const auto halfPageSize = this->_halfPageSize();

    if (_baseOffsetInPacketBits < halfPageSize.bits()) {
        _baseOffsetInPacketBits = 0;
    } else {
        _baseOffsetInPacketBits -= halfPageSize.bits();
    }

    this->_setEndOffsetInPacketBitsFromBaseOffset();
    this->_redrawContent();
}

void PacketDataView::isAsciiVisible(const bool isVisible)
{
    _isAsciiVisible = isVisible;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_redrawContent();
}

void PacketDataView::isEventRecordFirstPacketRegionEmphasized(const bool isEmphasized)
{
    _isEventRecordFirstPacketRegionEmphasized = isEmphasized;
    this->_redrawContent();
}

void PacketDataView::isHex(const bool isHex)
{
    _isHex = isHex;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_redrawContent();
}

} // namespace jacques
