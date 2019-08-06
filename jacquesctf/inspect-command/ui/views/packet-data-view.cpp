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
#include "message.hpp"
#include "content-packet-region.hpp"
#include "padding-packet-region.hpp"
#include "error-packet-region.hpp"
#include "inspect-screen.hpp"
#include "utils.hpp"

namespace jacques {

PacketDataView::PacketDataView(const Rectangle& rect,
                               const Stylist& stylist, State& state,
                               const InspectScreen::Bookmarks& bookmarks) :
    View {rect, "Packet data", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this},
    _bookmarks {&bookmarks}
{
    this->_setTitle();
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

void PacketDataView::_stateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DATA_STREAM_FILE_CHANGED ||
            msg == Message::ACTIVE_PACKET_CHANGED) {
        if (_state->hasActivePacketState()) {
            this->_setDataXAndRowSize();
            this->_setPrevCurNextOffsetInPacketBits();
            this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
        }

        this->_redrawContent();
    } else if (msg == Message::CUR_OFFSET_IN_PACKET_CHANGED) {
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
    const auto prevPacketRegion = packet.previousRegion(*curPacketRegion);

    if (prevPacketRegion) {
        _prevOffsetInPacketBits = prevPacketRegion->segment().offsetInPacketBits();
    } else {
        _prevOffsetInPacketBits = boost::none;
    }

    // next
    if (*curPacketRegion->segment().endOffsetInPacketBits() <
            packet.indexEntry().effectiveTotalSize()) {
        _nextOffsetInPacketBits = *curPacketRegion->segment().endOffsetInPacketBits();
    } else {
        _nextOffsetInPacketBits = boost::none;
    }
}

bool PacketDataView::_isCharSelected(const _Char& ch) const
{
    auto selectedPacketRegionIt = ch.packetRegionIt(_curOffsetInPacketBits);

    if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
        return true;
    }

    if (_prevOffsetInPacketBits) {
        selectedPacketRegionIt = ch.packetRegionIt(*_prevOffsetInPacketBits);

        if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
            return true;
        }
    }

    if (_nextOffsetInPacketBits) {
        selectedPacketRegionIt = ch.packetRegionIt(*_nextOffsetInPacketBits);

        if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
            return true;
        }
    }

    return false;
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

    if (_isAsciiVisible) {
        for (const auto& ch : _asciiChars) {
            if (this->_isCharSelected(ch)) {
                this->_drawUnselectedChar(ch);
            }
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

    if (_isAsciiVisible) {
        for (const auto& ch : _asciiChars) {
            if (this->_isCharSelected(ch)) {
                this->_drawChar(ch);
            }
        }
    }

    // update offset
    this->_drawOffsets();
}

void PacketDataView::_setDataXAndRowSize()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    std::array<char, 32> buf;
    const Size div = _isOffsetInBytes ? 8 : 1;
    auto totalSizeBits = _state->activePacketState().packetIndexEntry().effectiveTotalSize().bits();

    if (!_isOffsetInPacket) {
        totalSizeBits += _state->activePacketState().packetIndexEntry().offsetInDataStreamFileBits();
    }

    const auto maxOffset = static_cast<unsigned long long>(totalSizeBits) / div;

    if (_isOffsetInHex) {
        std::sprintf(buf.data(), "%llx", maxOffset);
    } else {
        std::strcpy(buf.data(), utils::sepNumber(maxOffset).c_str());
    }

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
            if (_isRowSizePowerOfTwo) {
                bytesPerRow /= 2;
            } else {
                --bytesPerRow;
            }

            break;
        }

        if (_isRowSizePowerOfTwo) {
            bytesPerRow *= 2;
        } else {
            ++bytesPerRow;
        }
    }

    if (bytesPerRow > _state->activePacketState().packetIndexEntry().effectiveTotalSize().bytes()) {
        bytesPerRow = _state->activePacketState().packetIndexEntry().effectiveTotalSize().bytes();
    }

    _dataX = offsetWidth + 1;
    _asciiCharsX = _dataX + bytesPerRow * (this->_charsPerByte() + 1);
    _rowSize = DataSize::fromBytes(bytesPerRow);
}

void PacketDataView::_drawSeparators() const
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    const auto endOffsetInPacketBits = _endOffsetInPacketBits + _rowSize.bits() - 1;
    const auto yEnd = (endOffsetInPacketBits - _baseOffsetInPacketBits) /
                      _rowSize.bits();

    this->_stylist().stdDim(*this);

    for (Index y = 0; y < yEnd; ++y) {
        this->_putChar({_dataX - 1, y}, ACS_VLINE);
    }

    if (!_isAsciiVisible) {
        return;
    }

    for (Index y = 0; y < yEnd; ++y) {
        this->_putChar({_asciiCharsX - 1, y}, ACS_VLINE);
    }
}

void PacketDataView::_drawOffsets() const
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    assert(_dataX > 0 && _rowSize > 0);

    const auto &curRegionSegment = _state->currentPacketRegion()->segment();
    const auto& packetTotalSize = _state->activePacketState().packetIndexEntry().effectiveTotalSize();
    const auto maxOffsetWidth = _dataX - 1;
    auto offsetInPacketBits = _baseOffsetInPacketBits;
    const Size div = _isOffsetInBytes ? 8 : 1;

    for (Index y = 0; y < this->contentRect().h; ++y) {
        if (offsetInPacketBits >= packetTotalSize) {
            // clear
            for (Index x = 0; x < _dataX - 1; ++x) {
                this->_putChar({x, y}, ' ');
            }
        } else {
            std::array<char, 32> buf;
            Index dispOffsetInPacketBits = offsetInPacketBits;

            if (!_isOffsetInPacket) {
                dispOffsetInPacketBits += _state->activePacketState().packetIndexEntry().offsetInDataStreamFileBits();
            }

            dispOffsetInPacketBits /= div;

            if (_isOffsetInHex) {
                std::sprintf(buf.data(), "%llx", dispOffsetInPacketBits);
            } else {
                std::strcpy(buf.data(),
                            utils::sepNumber(dispOffsetInPacketBits, ',').c_str());
            }

            const auto offsetWidth = std::strlen(buf.data());

            this->_moveCursor({0, y});

            const auto curRegionBaseOffsetBits = curRegionSegment.offsetInPacketBits() -
                                                 curRegionSegment.offsetInPacketBits() % _rowSize.bits();

            assert(curRegionSegment.endOffsetInPacketBits());

            const auto curRegionBaseEndOffsetBits = (*curRegionSegment.endOffsetInPacketBits() - 1) -
                                                    (*curRegionSegment.endOffsetInPacketBits() - 1) % _rowSize.bits();

            assert(curRegionBaseEndOffsetBits >= curRegionBaseOffsetBits);

            const auto selected = offsetInPacketBits >= curRegionBaseOffsetBits &&
                                  offsetInPacketBits <= curRegionBaseEndOffsetBits;

            this->_stylist().packetDataViewOffset(*this, selected);

            for (Index x = 0; x < maxOffsetWidth - offsetWidth; ++x) {
                this->_appendChar(' ');
            }

            this->_print("%s", buf.data());
            offsetInPacketBits += _rowSize.bits();
        }
    }
}

void PacketDataView::_setTitle()
{
    std::string title = "Packet data (offsets: ";

    if (_isOffsetInHex) {
        title += "hex";
    } else {
        title += "dec";
    }

    title += ", ";

    if (_isOffsetInBytes) {
        title += "bytes";
    } else {
        title += "bits";
    }

    title += ", in ";

    if (_isOffsetInPacket) {
        title += "packet";
    } else {
        title += "DSF";
    }

    title += ")";
    this->_title(title);
    this->_decorate();
}

void PacketDataView::_setCustomStyle(const _Char& ch) const
{
    if (ch.packetRegions.size() != 1) {
        if (ch.isPrintable) {
            this->_stylist().std(*this);
        } else {
            this->_stylist().stdDim(*this);
        }

        return;
    }

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
            if (ch.isPrintable) {
                this->_stylist().std(*this);
            } else {
                this->_stylist().stdDim(*this);
            }
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
    this->_setCustomStyle(ch);
    this->_putChar(ch.pt, ch.value);
}

void PacketDataView::_drawChar(const _Char& ch) const
{
    auto selectedPacketRegionIt = ch.packetRegionIt(_curOffsetInPacketBits);

    if (selectedPacketRegionIt != std::end(ch.packetRegions)) {
        if (ch.packetRegions.size() == 1) {
            this->_stylist().packetDataViewSelection(*this,
                                                     Stylist::PacketDataViewSelectionType::CURRENT);
        } else {
            this->_stylist().packetDataViewAuxSelection(*this,
                                                        Stylist::PacketDataViewSelectionType::CURRENT);
        }

        this->_putChar(ch.pt, ch.value);
        return;
    }

    if (ch.packetRegions.size() != 1) {
        this->_drawUnselectedChar(ch);
        return;
    }

    const auto& singlePacketRegion = *ch.packetRegions.front();

    if (_isPrevNextVisible) {
        if (_prevOffsetInPacketBits &&
                *_prevOffsetInPacketBits == singlePacketRegion.segment().offsetInPacketBits()) {
            this->_stylist().packetDataViewSelection(*this,
                                                     Stylist::PacketDataViewSelectionType::PREVIOUS);
            this->_putChar(ch.pt, ch.value);
            return;
        }

        if (_nextOffsetInPacketBits &&
                *_nextOffsetInPacketBits == singlePacketRegion.segment().offsetInPacketBits()) {
            this->_stylist().packetDataViewSelection(*this,
                                                     Stylist::PacketDataViewSelectionType::NEXT);
            this->_putChar(ch.pt, ch.value);
            return;
        }
    }

    this->_drawUnselectedChar(ch);
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

    for (const auto& ch : _asciiChars) {
        this->_drawChar(ch);
    }
}

void PacketDataView::_redrawContent()
{
    this->_clearContent();

    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_drawOffsets();
    this->_drawSeparators();
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

void PacketDataView::_setHexChars()
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

    for (auto& packetRegion : _packetRegions) {
        const auto bitArray = _state->activePacketState().packet().bitArray(*packetRegion);
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();

        bool isEventRecordFirst = false;

        if (packetRegion->scope() && packetRegion->scope()->eventRecord() &&
                packetRegion->scope()->eventRecord() != curEventRecord) {
            curEventRecord = packetRegion->scope()->eventRecord();
            isEventRecordFirst = true;
        }

        const auto startOffsetInPacketBits = std::max(firstBitOffsetInPacket,
                                                      _baseOffsetInPacketBits);
        const auto endOffsetInPacketBits = std::min(firstBitOffsetInPacket +
                                                    packetRegion->segment().size()->bits(),
                                                    _endOffsetInPacketBits);

        for (Index bitOffsetInPacket = startOffsetInPacketBits;
                bitOffsetInPacket < endOffsetInPacketBits;
                ++bitOffsetInPacket) {
            const auto indexInBitArray = bitOffsetInPacket -
                                         firstBitOffsetInPacket;

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
                    ch.packetRegions.back() != packetRegion.get()) {
                // associate character to packet region
                ch.packetRegions.push_back(packetRegion.get());
            }
        }
    }
}

void PacketDataView::_setAsciiChars()
{
    /*
     * Similar strategy to what we're doing in _setHexChars(), only here
     * the characters represent whole bytes, not nibbles.
     */
    assert((_baseOffsetInPacketBits & 7) == 0);

    const auto& packet = _state->activePacketState().packet();

    for (auto offsetInPacketBits = _baseOffsetInPacketBits;
            offsetInPacketBits < _endOffsetInPacketBits; offsetInPacketBits += 8) {
        _Char ch;

        ch.pt.y = (offsetInPacketBits - _baseOffsetInPacketBits) /
                  _rowSize.bits();
        ch.pt.x = _asciiCharsX + (offsetInPacketBits % _rowSize.bits()) / 8;

        const char value = static_cast<chtype>(*packet.data(offsetInPacketBits / 8));

        if (!std::isprint(value)) {
            ch.isPrintable = false;
            ch.value = ACS_BULLET;
        } else {
            ch.value = value;
        }

        _asciiChars.push_back(std::move(ch));
    }

    const EventRecord *curEventRecord = nullptr;

    for (const auto& packetRegion : _packetRegions) {
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();

        bool isEventRecordFirst = false;

        if (packetRegion->scope() && packetRegion->scope()->eventRecord() &&
                packetRegion->scope()->eventRecord() != curEventRecord) {
            curEventRecord = packetRegion->scope()->eventRecord();
            isEventRecordFirst = true;
        }

        const auto startOffsetInPacketBits = std::max(firstBitOffsetInPacket,
                                                      _baseOffsetInPacketBits);
        const auto endOffsetInPacketBits = std::min(firstBitOffsetInPacket +
                                                    packetRegion->segment().size()->bits(),
                                                    _endOffsetInPacketBits);

        for (Index bitOffsetInPacket = startOffsetInPacketBits;
                bitOffsetInPacket < endOffsetInPacketBits;
                ++bitOffsetInPacket) {
            const auto charIndex = ((bitOffsetInPacket / 8) -
                                    (_baseOffsetInPacketBits / 8));

            assert(charIndex < _asciiChars.size());

            // get existing character (created above)
            auto& ch = _asciiChars[charIndex];

            ch.isEventRecordFirst = isEventRecordFirst;

            if (ch.packetRegions.empty() ||
                    ch.packetRegions.back() != packetRegion.get()) {
                // associate character to packet region
                ch.packetRegions.push_back(packetRegion.get());
            }
        }
    }
}

void PacketDataView::_setBinaryChars()
{
    const EventRecord *curEventRecord = nullptr;

    for (const auto& packetRegion : _packetRegions) {
        const auto bitArray = _state->activePacketState().packet().bitArray(*packetRegion);
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();
        bool isEventRecordFirst = false;

        if (packetRegion->scope() && packetRegion->scope()->eventRecord() &&
                packetRegion->scope()->eventRecord() != curEventRecord) {
            curEventRecord = packetRegion->scope()->eventRecord();
            isEventRecordFirst = true;
        }

        const auto startOffsetInPacketBits = std::max(firstBitOffsetInPacket,
                                                      _baseOffsetInPacketBits);
        const auto endOffsetInPacketBits = std::min(firstBitOffsetInPacket +
                                                    packetRegion->segment().size()->bits(),
                                                    _endOffsetInPacketBits);

        for (Index bitOffsetInPacket = startOffsetInPacketBits;
                bitOffsetInPacket < endOffsetInPacketBits;
                ++bitOffsetInPacket) {
            const auto indexInBitArray = bitOffsetInPacket -
                                         firstBitOffsetInPacket;
            _Char ch;
            const auto bitLoc = bitArray.bitLocation(indexInBitArray);

            ch.isEventRecordFirst = isEventRecordFirst;
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
            ch.packetRegions.push_back(packetRegion.get());
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
    const auto& basePacketRegion = packet.regionAtOffsetInPacketBits(_baseOffsetInPacketBits);
    auto startingPacketRegion = packet.previousRegion(basePacketRegion);

    if (!startingPacketRegion) {
        startingPacketRegion = &basePacketRegion;
    }

    _packetRegions.clear();
    packet.appendRegions(_packetRegions,
                         startingPacketRegion->segment().offsetInPacketBits(),
                         _endOffsetInPacketBits);
    assert(!_packetRegions.empty());
    _chars.clear();

    if (_isDataInHex) {
        this->_setHexChars();
    } else {
        this->_setBinaryChars();
    }

    // set ASCII chars
    _asciiChars.clear();
    this->_setAsciiChars();
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

void PacketDataView::isPrevNextVisible(const bool isVisible)
{
    _isPrevNextVisible = isVisible;
    this->_redrawContent();
}

void PacketDataView::isEventRecordFirstPacketRegionEmphasized(const bool isEmphasized)
{
    _isEventRecordFirstPacketRegionEmphasized = isEmphasized;
    this->_redrawContent();
}

void PacketDataView::isDataInHex(const bool isDataInHex)
{
    _isDataInHex = isDataInHex;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_redrawContent();
}

void PacketDataView::centerSelection()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    this->_redrawContent();
}

void PacketDataView::isOffsetInHex(const bool isOffsetInHex)
{
    _isOffsetInHex = isOffsetInHex;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

void PacketDataView::isOffsetInBytes(const bool isOffsetInBytes)
{
    _isOffsetInBytes = isOffsetInBytes;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

void PacketDataView::isRowSizePowerOfTwo(const bool isPowerOfTwo)
{
    _isRowSizePowerOfTwo = isPowerOfTwo;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_redrawContent();
}

void PacketDataView::isOffsetInPacket(const bool isOffsetInPacket)
{
    _isOffsetInPacket = isOffsetInPacket;
    this->_setDataXAndRowSize();

    if (_state->hasActivePacketState()) {
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_curOffsetInPacketBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

} // namespace jacques
