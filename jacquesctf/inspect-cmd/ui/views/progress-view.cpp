/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <utility>
#include <variant>
#include <fmt/format.h>

#include "progress-view.hpp"
#include "utils.hpp"
#include "data/data-len.hpp"
#include "../stylist.hpp"

namespace jacques {

ProgressView::ProgressView(const Rect& rect, const std::string_view title,
                           const Stylist& stylist) :
    View {rect, title, DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void ProgressView::subjectPath(const boost::filesystem::path& path)
{
    _subjectPath = utils::escapeStr(path.string());
    _subjectText.clear();
    this->_invalidate();
}

void ProgressView::subjectText(const std::string_view text)
{
    _subjectPath = std::nullopt;
    _subjectText = utils::escapeStr(text);
    this->_invalidate();
}

void ProgressView::progress(const double ratio)
{
    _ratio = std::clamp(ratio, 0., 1.);
    this->_invalidate();
}

void ProgressView::rows(std::vector<LabelValRow> rows)
{
    _rows = std::move(rows);
    this->_invalidate();
}

void ProgressView::_drawSubject() const
{
    constexpr Index y = 1;

    if (!_subjectPath) {
        this->_style(Stylist::Role::PATH);
        this->_write({1, y}, _subjectText);
        return;
    }

    const auto [dirName, filename] = _formatPath(*_subjectPath, this->contentRect().w - 2);
    Index x = 1;

    if (!dirName.empty()) {
        this->_style(Stylist::Role::PATH);
        x = this->_write({x, y}, dirName);
        x = this->_write({x, y}, "/");
    }

    this->_style(Stylist::Role::PATH, true);
    this->_write({x, y}, filename);
}

void ProgressView::_drawBar() const
{
    constexpr Index y = 3;

    if (this->contentRect().w < 2) {
        return;
    }

    const auto barW = this->contentRect().w - 2;
    const auto progW = static_cast<Size>(_ratio * static_cast<double>(barW));

    this->_style(Stylist::Role::INVERSE);
    this->_fill({{1, y}, progW, 1});

    std::string rest;

    for (auto i = progW; i < barW; ++i) {
        rest += glyphs::CKBOARD;
    }

    this->_style(Stylist::Role::DIM);
    this->_write({1 + progW, y}, rest);
}

void ProgressView::_redrawContent()
{
    this->_clearContent();
    this->_drawSubject();
    this->_drawBar();
    this->_writeLabelValRows({1, 5}, _rows, Stylist::Role::STD, false);
}

namespace {

std::string formatLen(const DataLen& len)
{
    const auto fmtLen = std::get<FullFmtDataLen>(
        len.format(utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS));
    auto str = fmtLen.qty;

    if (fmtLen.extraBits) {
        str += '+' + *fmtLen.extraBits;
    }

    return str + ' ' + fmtLen.unit;
}

} // namespace

void setPktIndexBuildProgress(ProgressView& view, const DsFile& dsf,
                              const PktIndexEntry * const lastEntry)
{
    const auto offsetBytes = lastEntry ? lastEntry->offsetInDsFileBytes() : 0;

    view.subjectPath(dsf.path());

    if (dsf.fileLen().bytes() > 0) {
        view.progress(static_cast<double>(offsetBytes) /
                      static_cast<double>(dsf.fileLen().bytes()));
    } else {
        view.progress(0);
    }

    std::vector<LabelValRow> rows {
        {"Index:", utils::sepNumber(lastEntry ? lastEntry->natIndexInDsFile() : 0ULL, ',')},
        {"Offset:", formatLen(DataLen::fromBytes(offsetBytes))},
        {"Length:", formatLen(dsf.fileLen())},
    };

    if (lastEntry && lastEntry->seqNum()) {
        rows.push_back({"Seq num:", utils::sepNumber(*lastEntry->seqNum(), ',')});
    }

    view.rows(std::move(rows));
}

void setPktCheckpointsBuildProgress(ProgressView& view, const PktIndexEntry& pktIndexEntry,
                                    const Er * const lastEr)
{
    const auto& contentLen = pktIndexEntry.effectiveContentLen();

    view.subjectText(fmt::format("Packet #{}", pktIndexEntry.natIndexInDsFile()));

    if (!lastEr) {
        view.progress(0);
        view.rows({{"Length:", formatLen(contentLen)}});
        return;
    }

    const auto offsetInPktBits = lastEr->segment().offsetInPktBits();

    if (contentLen.bits() > 0) {
        view.progress(static_cast<double>(offsetInPktBits) /
                      static_cast<double>(contentLen.bits()));
    } else {
        view.progress(0);
    }

    std::vector<LabelValRow> rows {
        {"Index:", utils::sepNumber(lastEr->indexInPkt(), ',')},
        {"Offset:", formatLen(offsetInPktBits)},
        {"Length:", formatLen(contentLen)},
    };

    if (const auto ert = lastEr->type()) {
        if (ert->name()) {
            rows.push_back({"ERT name:", utils::escapeStr(*ert->name())});
        }

        rows.push_back({"ERT ID:", fmt::to_string(ert->id())});
    }

    view.rows(std::move(rows));
}

} // namespace jacques
