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

namespace jacques {

PacketDataView::PacketDataView(const Rectangle& rect,
                               const Stylist& stylist, State& state) :
    View {rect, "Packet data", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
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
        // bytes and spaces
        const auto dataWidth = bytesPerRow * 8 + bytesPerRow - 1;

        if (dataWidth + offsetWidth + 1 > this->contentRect().w) {
            bytesPerRow /= 2;
            break;
        }

        bytesPerRow *= 2;
    }

    _dataX = offsetWidth + 1;
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
    if (const auto region = dynamic_cast<const ContentPacketRegion *>(zone.packetRegion.get())) {
        this->_stylist().std(*this);
    } else if (const auto region = dynamic_cast<const PaddingPacketRegion *>(zone.packetRegion.get())) {
        this->_stylist().packetDataViewPadding(*this);
    } else if (const auto region = dynamic_cast<const ErrorPacketRegion *>(zone.packetRegion.get())) {
        this->_stylist().error(*this);
    } else {
        std::abort();
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

void PacketDataView::_redrawContent()
{
    this->_clearContent();

    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_drawOffsets();
    _zones.clear();
    this->_appendZones(_zones, _baseOffsetInPacketBits, _endOffsetInPacketBits);
    this->_drawAllZones();
    this->_hasMoreTop(_baseOffsetInPacketBits != 0);

    const auto effectiveTotalSizeBits = _state->activePacketState().packet().indexEntry().effectiveTotalSize().bits();

    this->_hasMoreBottom(_endOffsetInPacketBits < effectiveTotalSizeBits);
}

void PacketDataView::_appendZones(_Zones& zones,
                                  const Index startOffsetInPacketBits,
                                  const Index endOffsetInPacketBits)
{
    if (startOffsetInPacketBits == endOffsetInPacketBits) {
        return;
    }

    assert(startOffsetInPacketBits < endOffsetInPacketBits);

    // request packet regions
    assert(_state->hasActivePacketState());

    std::vector<PacketRegion::SPC> packetRegions;

    _state->activePacketState().packet().appendPacketRegions(packetRegions,
                                                             startOffsetInPacketBits,
                                                             endOffsetInPacketBits);
    assert(!packetRegions.empty());

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
            zones.push_back(std::move(zone));
        }
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

} // namespace jacques
