/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <yactfr/metadata/event-record-type.hpp>

#include "packet-checkpoints-build-progress-view.hpp"
#include "stylist.hpp"
#include "utils.hpp"

namespace jacques {

PacketCheckpointsBuildProgressView::PacketCheckpointsBuildProgressView(const Rectangle& rect,
                                                                       const Stylist& stylist) :
    View {
        rect,
        "Creating packet checkpoints...",
        DecorationStyle::BORDERS_EMPHASIZED, stylist
    }
{
}

void PacketCheckpointsBuildProgressView::_clearRow(const Index contentY)
{
    this->_stylist().std(*this);

    auto pos = Point {0, contentY};

    this->_putChar(pos, ' ');
    ++pos.x;

    while (pos.x < this->contentRect().w) {
        this->_appendChar(' ');
        ++pos.x;
    }
}

void PacketCheckpointsBuildProgressView::packetIndexEntry(const PacketIndexEntry& packetIndexEntry)
{
    _packetIndexEntry = &packetIndexEntry;
    _eventRecord = nullptr;
    this->_redrawContent();
}

void PacketCheckpointsBuildProgressView::eventRecord(const EventRecord& eventRecord)
{
    _eventRecord = &eventRecord;
    this->_drawProgress();
}

void PacketCheckpointsBuildProgressView::_drawPacketIndexEntry()
{
    if (!_packetIndexEntry) {
        return;
    }

    this->_clearRow(1);
    this->_stylist().packetIndexBuildProgressViewPath(*this, false);
    this->_moveAndPrint({1, 1}, "Packet #%llu",
                        _packetIndexEntry->natIndexInDataStream());
}

void PacketCheckpointsBuildProgressView::_drawProgress()
{
    if (!_packetIndexEntry || !_eventRecord) {
        return;
    }

    constexpr Index barY = 3;
    constexpr auto indexY = barY + 2;
    constexpr auto offsetInPacketBitsY = indexY + 1;
    constexpr auto sizeY = offsetInPacketBitsY + 1;
    constexpr auto ertNameY = sizeY + 1;
    constexpr auto ertIdY = ertNameY + 1;
    constexpr Index titleX = 1;
    constexpr auto infoX = titleX + 10;

    // bar
    this->_clearRow(barY);

    const auto barW = this->contentRect().w - 2;
    const auto fBarProgW = (static_cast<double>(_eventRecord->segment().offsetInPacketBits()) /
                            static_cast<double>(_packetIndexEntry->effectiveContentSize().bits())) *
                           static_cast<double>(barW);
    const auto barProgW = static_cast<Index>(fBarProgW);
    Index x = 1;

    this->_stylist().packetIndexBuildProgressViewBar(*this, true);

    for (; x < 1 + barProgW; ++x) {
        this->_putChar({x, barY}, ' ');
    }

    this->_stylist().packetIndexBuildProgressViewBar(*this, false);

    for (; x < 1 + barW; ++x) {
        this->_putChar({x, barY}, ACS_CKBOARD);
    }

    // index
    this->_clearRow(indexY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, indexY}, "Index:");
    this->_stylist().std(*this, true);
    this->_moveAndPrint({infoX, indexY}, "%s",
                        utils::sepNumber(static_cast<long long>(_eventRecord->indexInPacket()), ',').c_str());

    // offset
    this->_clearRow(offsetInPacketBitsY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, offsetInPacketBitsY}, "Offset:");
    this->_stylist().std(*this, true);

    auto sizeUnit = utils::formatSize(_eventRecord->segment().offsetInPacketBits(),
                                      utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                      ',');

    this->_moveAndPrint({infoX, offsetInPacketBitsY}, "%s %s",
                        sizeUnit.first.c_str(), sizeUnit.second.c_str());

    // size
    this->_clearRow(sizeY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, sizeY}, "Size:");
    this->_stylist().std(*this, true);
    sizeUnit = _packetIndexEntry->effectiveContentSize().format(utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                                ',');
    this->_moveAndPrint({infoX, sizeY}, "%s %s",
                        sizeUnit.first.c_str(), sizeUnit.second.c_str());

    // ERT name
    this->_clearRow(ertNameY);
    this->_clearRow(ertIdY);

    const auto ert = _eventRecord->type();

    if (!ert) {
        return;
    }

    if (ert->name()) {
        this->_stylist().std(*this);
        this->_moveAndPrint({titleX, ertNameY}, "ERT name:");
        this->_stylist().std(*this, true);
        this->_moveAndPrint({infoX, ertNameY}, "%s", ert->name()->c_str());
    }

    // ERT ID
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, ertIdY}, "ERT ID:");
    this->_stylist().std(*this, true);
    this->_moveAndPrint({infoX, ertIdY}, "%llu", ert->id());
}

void PacketCheckpointsBuildProgressView::_redrawContent()
{
    this->_clearContent();
    this->_drawPacketIndexEntry();
    this->_drawProgress();
}

void PacketCheckpointsBuildProgressView::_resized()
{
    // TODO
}

} // namespace jacques
