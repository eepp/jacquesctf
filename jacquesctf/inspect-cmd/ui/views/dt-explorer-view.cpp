/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdlib>
#include <functional>
#include <string>
#include <fmt/format.h>

#include "abstract-dt-details.hpp"
#include "dt-explorer-view.hpp"
#include "dt-details.hpp"

namespace jacques {

DtExplorerView::DtExplorerView(const Rect& rect, const Stylist& stylist) :
    ScrollView {rect, "", DecorationStyle::BORDERS, stylist}
{
}

void DtExplorerView::dst(const yactfr::DataStreamType& dst, const bool showErts)
{
    if (&dst == _dst && showErts == _dstShowErts) {
        return;
    }

    _details.pktHeader.clear();
    _details.pktContext.clear();
    _details.ertHeader.clear();
    _details.ertFirstCtx.clear();
    _rows.clear();
    _singleDt = nullptr;
    _ert = nullptr;
    _dst = &dst;
    _dstShowErts = showErts;

    if (const auto traceType = dst.traceType(); traceType->packetHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PacketHeader});
        this->_appendDetailsRow(*traceType->packetHeaderType(), _details.pktHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dst.packetContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PacketContext});
        this->_appendDetailsRow(*dst.packetContextType(), _details.pktContext);
        _rows.push_back(_EmptyRow {});
    }

    if (showErts) {
        if (dst.eventRecordHeaderType()) {
            _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordHeader});
            this->_appendDetailsRow(*dst.eventRecordHeaderType(), _details.ertHeader);
            _rows.push_back(_EmptyRow {});
        }

        if (dst.eventRecordCommonContextType()) {
            _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordCommonContext});
            this->_appendDetailsRow(*dst.eventRecordCommonContextType(), _details.ertFirstCtx);
            _rows.push_back(_EmptyRow {});
        }
    }

    if (!_rows.empty()) {
        // remove trailing empty row
        _rows.pop_back();
    }

    this->_index(0);
    this->_rowCount(_rows.size());
    this->clearHighlight();

    this->_title(fmt::format("Data stream type data types: ({})", dst.id()));
}

void DtExplorerView::ert(const yactfr::EventRecordType& ert)
{
    if (&ert == _ert) {
        return;
    }

    _details.ertHeader.clear();
    _details.ertFirstCtx.clear();
    _details.ertSecondCtx.clear();
    _details.ertPayload.clear();
    _singleDt = nullptr;
    _ert = &ert;
    _dst = nullptr;

    auto& dst = *ert.dataStreamType();

    _rows.clear();

    if (dst.eventRecordHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordHeader});
        this->_appendDetailsRow(*dst.eventRecordHeaderType(), _details.ertHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dst.eventRecordCommonContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordCommonContext});
        this->_appendDetailsRow(*dst.eventRecordCommonContextType(), _details.ertFirstCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (ert.specificContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordSpecificContext});
        this->_appendDetailsRow(*ert.specificContextType(), _details.ertSecondCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (ert.payloadType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EventRecordPayload});
        this->_appendDetailsRow(*ert.payloadType(), _details.ertPayload);
        _rows.push_back(_EmptyRow {});
    }

    if (!_rows.empty()) {
        // remove trailing empty row
        _rows.pop_back();
    }

    this->_index(0);
    this->_rowCount(_rows.size());
    this->clearHighlight();

    if (ert.name()) {
        this->_title(fmt::format("Event record type data types: `{}` ({})", *ert.name(),
                                 ert.id()));
    } else {
        this->_title(fmt::format("Event record type data types: ({})", ert.id()));
    }
}

void DtExplorerView::singleDt(const yactfr::DataType& dt, const yactfr::Scope scope)
{
    if (&dt == _singleDt) {
        return;
    }

    _singleDt = &dt;
    _ert = nullptr;
    _dst = nullptr;
    _singleDtDetails.clear();
    _rows.clear();
    _rows.push_back(_ScopeSubtitleRow {scope});
    this->_appendDetailsRow(dt, _singleDtDetails);
    this->_index(0);
    this->_rowCount(_rows.size());
    this->clearHighlight();

    std::string title {"Data type: "};

    switch (scope) {
    case yactfr::Scope::PacketHeader:
        title += "packet header";
        break;

    case yactfr::Scope::PacketContext:
        title += "packet context";
        break;

    case yactfr::Scope::EventRecordHeader:
        title += "event record header";
        break;

    case yactfr::Scope::EventRecordCommonContext:
        title += "event record common context";
        break;

    case yactfr::Scope::EventRecordSpecificContext:
        title += "event record specific context";
        break;

    case yactfr::Scope::EventRecordPayload:
        title += "event record payload";
        break;
    }

    this->_title(title);
}

void DtExplorerView::reset()
{
    this->_title("Data type");
    _singleDt = nullptr;
    _ert = nullptr;
    _dst = nullptr;
    _rows.clear();
    this->_rowCount(0);
    this->clearHighlight();
}

void DtExplorerView::highlightDt(const yactfr::DataType& dt)
{
    _highlight = &dt;
    this->_invalidate();
}

void DtExplorerView::clearHighlight()
{
    _highlight = nullptr;
    this->_invalidate();
}

void DtExplorerView::centerHighlight()
{
    if (!_highlight) {
        return;
    }

    if (this->_rowCount() <= this->contentRect().h) {
        // all rows are visible
        return;
    }

    Index highlightedRow;

    for (highlightedRow = 0; highlightedRow < _rows.size(); ++highlightedRow) {
        auto& row = _rows[highlightedRow];

        if (const auto detailsRow = std::get_if<const AbstractDtDetails *>(&row)) {
            const auto details = *detailsRow;

            if (const auto dtDetails = dynamic_cast<const DtDetails *>(details)) {
                if (&dtDetails->dt() == _highlight) {
                    break;
                }
            }
        }
    }

    if (highlightedRow == this->_rowCount()) {
        // not found
        return;
    }

    const auto newIndex = static_cast<long long>(highlightedRow - this->contentRect().h / 2);

    if (newIndex < 0) {
        // row is in the first half
        this->_index(0);
        return;
    }

    if (newIndex >= static_cast<long long>(this->_rowCount() - this->contentRect().h)) {
        // row is in the last half
        this->_index(this->_rowCount() - this->contentRect().h);
        return;
    }

    this->_index(static_cast<Index>(newIndex));
}

void DtExplorerView::_drawScopeSubtitleRow(const Index index, const std::string& text)
{
    this->_style(Stylist::Role::STD, true);
    this->_write({0, this->_contentRectYFromIndex(index)}, text);
}

void DtExplorerView::_drawRows()
{
    this->_style(Stylist::Role::STD);
    this->_clearContent();

    if (this->_rowCount() == 0) {
        this->_style(Stylist::Role::STD, true);

        const std::string msg {"Nothing to show here!"};

        this->_write({this->contentRect().w / 2 - msg.size() / 2, this->contentRect().h / 2}, msg);
        return;
    }

    assert(this->_index() < this->_rowCount());

    for (Index index = this->_index(); index < this->_index() + this->contentRect().h; ++index) {
        if (index >= _rows.size()) {
            return;
        }

        const auto y = this->_contentRectYFromIndex(index);
        auto& row = _rows[index];

        if (const auto subRow = std::get_if<_ScopeSubtitleRow>(&row)) {
            const auto scopeName = std::invoke([subRow] {
                switch (subRow->scope) {
                case yactfr::Scope::PacketHeader:
                    return "Packet header";

                case yactfr::Scope::PacketContext:
                    return "Packet context";

                case yactfr::Scope::EventRecordHeader:
                    return "Event record header";

                case yactfr::Scope::EventRecordCommonContext:
                    return "Event record common context";

                case yactfr::Scope::EventRecordSpecificContext:
                    return "Event record specific context";

                case yactfr::Scope::EventRecordPayload:
                    return "Event record payload";
                }

                std::abort();
            });

            this->_style(Stylist::Role::TITLE);
            this->_write({0, y}, "{} type:", scopeName);
        } else if (const auto detailsRow = std::get_if<const AbstractDtDetails *>(&row)) {
            const auto details = *detailsRow;
            bool highlighted = false;

            if (const auto dtDetails = dynamic_cast<const DtDetails *>(details)) {
                if (&dtDetails->dt() == _highlight) {
                    highlighted = true;
                    this->_style(Stylist::Role::SEL);
                    this->_clearRow(y);
                }
            }

            if (!highlighted) {
                this->_style(Stylist::Role::STD);
            }

            this->_moveCursor({2, y});
            details->renderLine(this->_plane(), this->contentRect().w - 2, !highlighted);
        }
    }
}

void DtExplorerView::_appendDetailsRow(const yactfr::DataType& dt, _Details& details)
{
    dtDetailsFromDt(dt, this->_stylist(), details);

    for (auto& detailsUp : details) {
        _rows.push_back(detailsUp.get());
    }
}

} // namespace jacques
