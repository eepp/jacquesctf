/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "pkt-checkpoints-build-progress-view.hpp"
#include "../stylist.hpp"
#include "utils.hpp"

namespace jacques {

PktCheckpointsBuildProgressView::PktCheckpointsBuildProgressView(const Rect& rect,
                                                                 const Stylist& stylist) :
    View {rect, "Creating packet checkpoints...", DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void PktCheckpointsBuildProgressView::_clearRow(const Index contentY)
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

void PktCheckpointsBuildProgressView::pktIndexEntry(const PktIndexEntry& pktIndexEntry)
{
    _pktIndexEntry = &pktIndexEntry;
    _er = nullptr;
    this->_redrawContent();
}

void PktCheckpointsBuildProgressView::er(const Er& er)
{
    _er = &er;
    this->_drawProgress();
}

void PktCheckpointsBuildProgressView::_drawPktIndexEntry()
{
    if (!_pktIndexEntry) {
        return;
    }

    this->_clearRow(1);
    this->_stylist().pktIndexBuildProgressViewPath(*this, false);
    this->_moveAndPrint({1, 1}, "Packet #%llu", _pktIndexEntry->natIndexInDsFile());
}

void PktCheckpointsBuildProgressView::_drawProgress()
{
    if (!_pktIndexEntry || !_er) {
        return;
    }

    constexpr Index barY = 3;
    constexpr auto indexY = barY + 2;
    constexpr auto offsetInPktBitsY = indexY + 1;
    constexpr auto sizeY = offsetInPktBitsY + 1;
    constexpr auto ertNameY = sizeY + 1;
    constexpr auto ertIdY = ertNameY + 1;
    constexpr Index titleX = 1;
    constexpr auto infoX = titleX + 10;

    // bar
    this->_clearRow(barY);

    const auto barW = this->contentRect().w - 2;
    const auto fBarProgW = (static_cast<double>(_er->segment().offsetInPktBits()) /
                            static_cast<double>(_pktIndexEntry->effectiveContentLen().bits())) *
                           static_cast<double>(barW);
    const auto barProgW = static_cast<Index>(fBarProgW);
    Index x = 1;

    this->_stylist().pktIndexBuildProgressViewBar(*this, true);

    for (; x < 1 + barProgW; ++x) {
        this->_putChar({x, barY}, ' ');
    }

    this->_stylist().pktIndexBuildProgressViewBar(*this, false);

    for (; x < 1 + barW; ++x) {
        this->_putChar({x, barY}, ACS_CKBOARD);
    }

    // index
    this->_clearRow(indexY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, indexY}, "Index:");
    this->_stylist().std(*this, true);
    this->_moveAndPrint({infoX, indexY}, "%s",
                        utils::sepNumber(static_cast<long long>(_er->indexInPkt()), ',').c_str());

    // offset
    this->_clearRow(offsetInPktBitsY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, offsetInPktBitsY}, "Offset:");
    this->_stylist().std(*this, true);

    auto lenUnit = utils::formatLen(_er->segment().offsetInPktBits(),
                                    utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS, ',');

    this->_moveAndPrint({infoX, offsetInPktBitsY}, "%s %s", lenUnit.first.c_str(),
                        lenUnit.second.c_str());

    // size
    this->_clearRow(sizeY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, sizeY}, "Length:");
    this->_stylist().std(*this, true);
    lenUnit = _pktIndexEntry->effectiveContentLen().format(utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                           ',');
    this->_moveAndPrint({infoX, sizeY}, "%s %s", lenUnit.first.c_str(), lenUnit.second.c_str());

    // ERT name
    this->_clearRow(ertNameY);
    this->_clearRow(ertIdY);

    const auto ert = _er->type();

    if (!ert) {
        return;
    }

    if (ert->name()) {
        this->_stylist().std(*this);
        this->_moveAndPrint({titleX, ertNameY}, "ERT name:");
        this->_stylist().std(*this, true);
        this->_moveAndPrint({infoX, ertNameY}, "%s", utils::escapeStr(*ert->name()).c_str());
    }

    // ERT ID
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, ertIdY}, "ERT ID:");
    this->_stylist().std(*this, true);
    this->_moveAndPrint({infoX, ertIdY}, "%llu", ert->id());
}

void PktCheckpointsBuildProgressView::_redrawContent()
{
    this->_clearContent();
    this->_drawPktIndexEntry();
    this->_drawProgress();
}

void PktCheckpointsBuildProgressView::_resized()
{
    // TODO
}

} // namespace jacques
