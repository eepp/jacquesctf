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

    const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize().bits();

    assert(offsetInPacketBits < effectiveTotalSizeBits);

    const auto rowOffsetInPacketBits = offsetInPacketBits -
                                       (offsetInPacketBits % _rowSize.bits());
    const auto halfPageSize = this->_halfPageSize();

    if (rowOffsetInPacketBits < halfPageSize.bits()) {
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
    if (_curOffsetInPacketBits > 0) {
        if (curPacketRegion->previousPacketRegionOffsetInPacketBits()) {
            _prevOffsetInPacketBits = *curPacketRegion->previousPacketRegionOffsetInPacketBits();
        } else {
            const auto& prevPacketRegion = packet.packetRegionAtOffsetInPacketBits(_curOffsetInPacketBits - 1);

            _prevOffsetInPacketBits = prevPacketRegion.segment().offsetInPacketBits();
        }
    } else {
        _prevOffsetInPacketBits = boost::none;
    }

    // next
    if (curPacketRegion->segment().endOffsetInPacketBits() <
            packet.indexEntry().effectiveTotalSize().bits()) {
        _nextOffsetInPacketBits = curPacketRegion->segment().endOffsetInPacketBits();
    } else {
        _nextOffsetInPacketBits = boost::none;
    }
}

bool PacketDataView::_isZoneSelected(const _Zone& zone) const
{
    const auto zoneOffsetInPacketBits = zone.packetRegion->segment().offsetInPacketBits();

    return zoneOffsetInPacketBits == _curOffsetInPacketBits ||
           (_prevOffsetInPacketBits &&
            zoneOffsetInPacketBits == *_prevOffsetInPacketBits) ||
           (_nextOffsetInPacketBits &&
            zoneOffsetInPacketBits == *_nextOffsetInPacketBits);
}

void PacketDataView::_updateSelection()
{
    assert(_state->hasActivePacketState());

    if (_state->curOffsetInPacketBits() < _baseOffsetInPacketBits ||
            _state->curOffsetInPacketBits() >= _endOffsetInPacketBits ||
            _zones.empty()) {
        // outside the current zones: reset base offset
        this->_setBaseAndEndOffsetInPacketBitsFromOffset(_state->curOffsetInPacketBits());
        this->_setPrevCurNextOffsetInPacketBits();
        this->_redrawContent();
        return;
    }

    // "erase" currently selected zones
    for (const auto& zone : _zones) {
        if (this->_isZoneSelected(zone)) {
            this->_drawUnselectedZone(zone);
        }
    }

    // draw new selected zones
    this->_setPrevCurNextOffsetInPacketBits();

    for (const auto& zone : _zones) {
        if (this->_isZoneSelected(zone)) {
            this->_drawZone(zone);
        }
    }

    // update offset
    this->_drawOffsets();

    // update ASCII chars
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
        auto dataWidth = bytesPerRow * 9 - 1;

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
    _asciiCharsX = _dataX + bytesPerRow * 9;
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
        if (offsetInPacketBits >= packetTotalSize.bits()) {
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
                                  _curOffsetInPacketBits < offsetInPacketBits + _rowSize.bits();

            this->_stylist().packetDataViewOffset(*this, selected);

            for (Index x = 0; x < maxOffsetWidth - offsetWidth; ++x) {
                this->_appendChar(' ');
            }

            this->_print("%s", buf.data());
            offsetInPacketBits += _rowSize.bits();
        }
    }
}

void PacketDataView::_drawZoneBits(const _Zone& zone) const
{
    for (const auto& bit : zone.bits) {
        this->_putChar(bit.pt, bit.value);
    }
}

void PacketDataView::_drawUnselectedZone(const _Zone& zone) const
{
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

    if (bookmarks) {
        for (Index id = 0; id < bookmarks->size(); ++id) {
            if (bookmarks->at(id) && *(bookmarks->at(id)) ==
                                     zone.packetRegion->segment().offsetInPacketBits()) {
                this->_stylist().packetDataViewBookmark(*this, id);
                bookmark = true;
                break;
            }
        }
    }

    if (!bookmark) {
        if (const auto region = dynamic_cast<const ContentPacketRegion *>(zone.packetRegion.get())) {
            this->_stylist().std(*this);
        } else if (const auto region = dynamic_cast<const PaddingPacketRegion *>(zone.packetRegion.get())) {
            this->_stylist().packetDataViewPadding(*this);
        } else if (const auto region = dynamic_cast<const ErrorPacketRegion *>(zone.packetRegion.get())) {
            this->_stylist().error(*this);
        } else {
            std::abort();
        }
    }

    this->_drawZoneBits(zone);
}

void PacketDataView::_drawZone(const _Zone& zone) const
{
    const auto zoneOffsetInPacketBits = zone.packetRegion->segment().offsetInPacketBits();

    if (zoneOffsetInPacketBits == _curOffsetInPacketBits) {
        this->_stylist().packetDataViewSelection(*this,
                                                 Stylist::PacketDataViewSelectionType::CURRENT);
    } else if (_prevOffsetInPacketBits &&
            zoneOffsetInPacketBits == *_prevOffsetInPacketBits) {
        this->_stylist().packetDataViewSelection(*this,
                                                 Stylist::PacketDataViewSelectionType::PREVIOUS);
    } else if (_nextOffsetInPacketBits &&
            zoneOffsetInPacketBits == *_nextOffsetInPacketBits) {
        this->_stylist().packetDataViewSelection(*this,
                                                 Stylist::PacketDataViewSelectionType::NEXT);
    } else {
        this->_drawUnselectedZone(zone);
        return;
    }

    this->_drawZoneBits(zone);
}

void PacketDataView::_drawAllZones() const
{
    for (const auto& zone : _zones) {
        this->_drawZone(zone);
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
    this->_setZonesAndAsciiChars();
    this->_drawAllZones();
    this->_drawAllAsciiChars();
    this->_hasMoreTop(_baseOffsetInPacketBits != 0);

    const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize().bits();

    this->_hasMoreBottom(_endOffsetInPacketBits < effectiveTotalSizeBits);
}

void PacketDataView::_setZonesAndAsciiChars()
{
    if (_baseOffsetInPacketBits == _endOffsetInPacketBits) {
        return;
    }

    assert(_baseOffsetInPacketBits < _endOffsetInPacketBits);
    assert(_state->hasActivePacketState());

    // set zones
    std::vector<PacketRegion::SPC> packetRegions;
    auto& packet = _state->activePacketState().packet();

    packet.appendPacketRegions(packetRegions, _baseOffsetInPacketBits,
                               _endOffsetInPacketBits);
    assert(!packetRegions.empty());
    _zones.clear();

    for (auto& packetRegion : packetRegions) {
        const auto bitArray = _state->activePacketState().packet().bitArray(*packetRegion);
        const auto firstBitOffsetInPacket = packetRegion->segment().offsetInPacketBits();

        _Zone zone;

        for (Index indexInBitArray = 0;
                indexInBitArray < packetRegion->segment().size().bits();
                ++indexInBitArray) {
            const auto bitOffsetInPacket = firstBitOffsetInPacket +
                                           indexInBitArray;

            if (bitOffsetInPacket < _baseOffsetInPacketBits ||
                    bitOffsetInPacket >= _endOffsetInPacketBits) {
                // not visible
                continue;
            }

            _Zone::Bit bit;
            const auto bitLoc = bitArray.bitLocation(indexInBitArray);

            bit.value = '0' + bitArray[bitLoc];
            bit.pt.y = (bitOffsetInPacket - _baseOffsetInPacketBits) /
                       _rowSize.bits();

            const auto byteIndex = (bitOffsetInPacket % _rowSize.bits()) / 8;

            /*
             * 28000 00101101 11010010 11101010 00010010
             *       ^ _dataX [6]      ^ + byteIndex * 9 [+ 2 * 9]
             *                              ^ + 7 - bitLoc.bitIndexInByte [+ 7 - 2]
             */
            bit.pt.x = _dataX + byteIndex * 9 + 7 - bitLoc.bitIndexInByte();
            zone.bits.push_back(bit);
        }

        if (!zone.bits.empty()) {
            zone.packetRegion = std::move(packetRegion);
            _zones.push_back(std::move(zone));
        }
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

} // namespace jacques
