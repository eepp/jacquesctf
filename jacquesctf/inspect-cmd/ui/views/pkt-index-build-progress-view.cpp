/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "view.hpp"
#include "utils.hpp"
#include "data/ds-file.hpp"
#include "data/pkt-index-entry.hpp"
#include "pkt-index-build-progress-view.hpp"
#include "../stylist.hpp"

namespace jacques {

PktIndexBuildProgressView::PktIndexBuildProgressView(const Rect& rect, const Stylist& stylist) :
    View {rect, "Building packet indexes...", DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void PktIndexBuildProgressView::_clearRow(const Index y)
{
    this->_stylist().std(*this);

    auto pos = Point {0, y};

    this->_putChar(pos, ' ');
    ++pos.x;

    while (pos.x < this->contentRect().w) {
        this->_appendChar(' ');
        ++pos.x;
    }
}

void PktIndexBuildProgressView::dsFile(const DsFile& dsf)
{
    _dsf = &dsf;
    this->_drawFile();
    _index = 0;
    _offsetBytes = 0;
    _seqNum = boost::none;
    this->_drawProgress();
}

void PktIndexBuildProgressView::pktIndexEntry(const PktIndexEntry& entry)
{
    _index = entry.indexInDsFile();
    _offsetBytes = entry.offsetInDsFileBytes();
    _seqNum = entry.seqNum();
    this->_drawProgress();
}

void PktIndexBuildProgressView::_drawFile()
{
    if (!_dsf) {
        return;
    }

    constexpr auto y = 1;

    this->_clearRow(y);
    std::string dirName, filename;

    std::tie(dirName, filename) = utils::formatPath(utils::escapeStr(_dsf->path().string()),
                                                    this->contentRect().w - 2);

    Index filenameX = 1;

    if (!dirName.empty()) {
        this->_stylist().pktIndexBuildProgressViewPath(*this, false);
        this->_moveAndPrint({filenameX, y}, "%s/", dirName.c_str());
        filenameX += dirName.size() + 1;
    }

    this->_stylist().pktIndexBuildProgressViewPath(*this, true);
    this->_moveAndPrint({filenameX, y}, "%s", filename.c_str());
}

void PktIndexBuildProgressView::_drawProgress()
{
    if (!_dsf) {
        return;
    }

    constexpr Index barY = 3;
    constexpr auto indexY = barY + 2;
    constexpr auto offsetY = indexY + 1;
    constexpr auto sizeY = offsetY + 1;
    constexpr auto seqNumY = sizeY + 1;
    constexpr Index titleX = 1;
    constexpr auto infoX = titleX + 9;

    // bar
    this->_clearRow(indexY);

    const auto barW = this->contentRect().w - 2;
    double fBarProgW = 0;

    if (_dsf->fileLen().bytes() > 0) {
        fBarProgW = (static_cast<double>(_offsetBytes) /
                     static_cast<double>(_dsf->fileLen().bytes())) *
                    static_cast<double>(barW);
    }

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
                        utils::sepNumber(static_cast<long long>(_index), ',').c_str());

    // offset
    this->_clearRow(offsetY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, offsetY}, "Offset:");
    this->_stylist().std(*this, true);

    {
        const auto lenUnit = utils::formatLen(_offsetBytes * 8,
                                              utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS, ',');

        this->_moveAndPrint({infoX, offsetY}, "%s %s", lenUnit.first.c_str(),
                            lenUnit.second.c_str());
    }

    this->_clearRow(sizeY);
    this->_stylist().std(*this);
    this->_moveAndPrint({titleX, sizeY}, "Size:");
    this->_stylist().std(*this, true);

    {
        const auto lenUnit = _dsf->fileLen().format(utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                    ',');

        this->_moveAndPrint({infoX, sizeY}, "%s %s", lenUnit.first.c_str(), lenUnit.second.c_str());
    }

    // sequence number
    this->_clearRow(seqNumY);

    if (_seqNum) {
        this->_stylist().std(*this);
        this->_moveAndPrint({titleX, seqNumY}, "Seq num:");
        this->_stylist().std(*this, true);
        this->_moveAndPrint({infoX, seqNumY}, "%15s",
                            utils::sepNumber(static_cast<long long>(*_seqNum), ',').c_str());
    }
}

void PktIndexBuildProgressView::_redrawContent()
{
    this->_drawFile();
    this->_drawProgress();
}

void PktIndexBuildProgressView::_resized()
{
    // TODO
}

} // namespace jacques
