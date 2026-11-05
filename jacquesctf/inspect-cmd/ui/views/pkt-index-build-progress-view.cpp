/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "view.hpp"
#include "utils.hpp"
#include "data/ds-file.hpp"
#include "data/pkt-index-entry.hpp"
#include "pkt-index-build-progress-view.hpp"
#include "../stylist.hpp"
#include "../term.hpp"

namespace jacques {

PktIndexBuildProgressView::PktIndexBuildProgressView(const Rect& rect, const Stylist& stylist) :
    View {rect, "Building packet indexes…", DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void PktIndexBuildProgressView::_clearRow(const Index y)
{
    this->_style(Stylist::Role::STD);

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
    _index = 0;
    _offsetBytes = 0;
    _seqNum = std::nullopt;
    this->_invalidate();
}

void PktIndexBuildProgressView::pktIndexEntry(const PktIndexEntry& entry)
{
    _index = entry.indexInDsFile();
    _offsetBytes = entry.offsetInDsFileBytes();
    _seqNum = entry.seqNum();
    this->_invalidate();
}

void PktIndexBuildProgressView::_drawFile()
{
    if (!_dsf) {
        return;
    }

    constexpr auto y = 1;

    this->_clearRow(y);
    const auto [dirName, filename] = _formatPath(utils::escapeStr(_dsf->path().string()),
                                                 this->contentRect().w - 2);

    Index filenameX = 1;

    if (!dirName.empty()) {
        this->_style(Stylist::Role::PATH);
        this->_moveAndPrint({filenameX, y}, "%s/", dirName.c_str());
        filenameX += term::strWidth(dirName) + 1;
    }

    this->_style(Stylist::Role::PATH, true);
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

    this->_style(Stylist::Role::INVERSE);

    for (; x < 1 + barProgW; ++x) {
        this->_putChar({x, barY}, ' ');
    }

    this->_style(Stylist::Role::DIM);

    for (; x < 1 + barW; ++x) {
        this->_putChar({x, barY}, glyphs::CKBOARD);
    }

    // index
    this->_clearRow(indexY);
    this->_style(Stylist::Role::STD);
    this->_moveAndPrint({titleX, indexY}, "Index:");
    this->_style(Stylist::Role::STD, true);
    this->_moveAndPrint({infoX, indexY}, "%s",
                        utils::sepNumber(static_cast<long long>(_index), ',').c_str());

    // offset
    this->_clearRow(offsetY);
    this->_style(Stylist::Role::STD);
    this->_moveAndPrint({titleX, offsetY}, "Offset:");
    this->_style(Stylist::Role::STD, true);

    {
        const auto [len, unit] = utils::formatLen(_offsetBytes * 8,
                                                  utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                  ',');

        this->_moveAndPrint({infoX, offsetY}, "%s %s", len.c_str(), unit.c_str());
    }

    this->_clearRow(sizeY);
    this->_style(Stylist::Role::STD);
    this->_moveAndPrint({titleX, sizeY}, "Length:");
    this->_style(Stylist::Role::STD, true);

    {
        const auto [len, unit] = _dsf->fileLen().format(utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                                        ',');

        this->_moveAndPrint({infoX, sizeY}, "%s %s", len.c_str(), unit.c_str());
    }

    // sequence number
    this->_clearRow(seqNumY);

    if (_seqNum) {
        this->_style(Stylist::Role::STD);
        this->_moveAndPrint({titleX, seqNumY}, "Seq num:");
        this->_style(Stylist::Role::STD, true);
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
