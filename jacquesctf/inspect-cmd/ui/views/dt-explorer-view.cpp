/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <sstream>
#include <yactfr/metadata/event-record-type.hpp>
#include <yactfr/metadata/data-stream-type.hpp>
#include <boost/variant.hpp>

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
    _details.pktHeader.clear();
    _details.pktContext.clear();
    _details.ertHeader.clear();
    _details.ertFirstCtx.clear();
    _rows.clear();
    _singleDt = nullptr;
    _ert = nullptr;

    const auto traceType = dst.traceType();

    if (traceType->packetHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PACKET_HEADER});
        this->_appendDetailsRow(*traceType->packetHeaderType(), _details.pktHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dst.packetContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PACKET_CONTEXT});
        this->_appendDetailsRow(*dst.packetContextType(), _details.pktContext);
        _rows.push_back(_EmptyRow {});
    }

    if (showErts) {
        if (dst.eventRecordHeaderType()) {
            _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_HEADER});
            this->_appendDetailsRow(*dst.eventRecordHeaderType(), _details.ertHeader);
            _rows.push_back(_EmptyRow {});
        }

        if (dst.eventRecordFirstContextType()) {
            _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT});
            this->_appendDetailsRow(*dst.eventRecordFirstContextType(), _details.ertFirstCtx);
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

    std::ostringstream ss;

    ss << "Data stream type data types: " << "(" << dst.id() << ")";
    this->_title(ss.str());
    this->redraw();
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

    auto& dst = *ert.dataStreamType();

    _rows.clear();

    if (dst.eventRecordHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_HEADER});
        this->_appendDetailsRow(*dst.eventRecordHeaderType(), _details.ertHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dst.eventRecordFirstContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT});
        this->_appendDetailsRow(*dst.eventRecordFirstContextType(), _details.ertFirstCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (ert.secondContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT});
        this->_appendDetailsRow(*ert.secondContextType(), _details.ertSecondCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (ert.payloadType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_PAYLOAD});
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

    std::ostringstream ss;

    ss << "Event record type data types: ";

    if (ert.name()) {
        ss << '`' << *ert.name() << "` ";
    }

    ss << "(" << ert.id() << ")";
    this->_title(ss.str());
    this->redraw();
}

void DtExplorerView::singleDt(const yactfr::DataType& dt, const yactfr::Scope scope)
{
    if (&dt == _singleDt) {
        return;
    }

    _singleDt = &dt;
    _ert = nullptr;
    _singleDtDetails.clear();
    _rows.clear();
    _rows.push_back(_ScopeSubtitleRow {scope});
    this->_appendDetailsRow(dt, _singleDtDetails);
    this->_index(0);
    this->_rowCount(_rows.size());
    this->clearHighlight();

    std::string title {"Data type: "};

    switch (scope) {
    case yactfr::Scope::PACKET_HEADER:
        title += "packet header";
        break;

    case yactfr::Scope::PACKET_CONTEXT:
        title += "packet context";
        break;

    case yactfr::Scope::EVENT_RECORD_HEADER:
        title += "event record header";
        break;

    case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
        title += "event record first context";
        break;

    case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
        title += "event record second context";
        break;

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        title += "event record payload";
        break;
    }

    this->_title(title);
    this->redraw();
}

void DtExplorerView::reset()
{
    this->_title("Data type");
    _singleDt = nullptr;
    _ert = nullptr;
    _rows.clear();
    this->_rowCount(0);
    this->clearHighlight();
    this->redraw();
}

void DtExplorerView::highlightDt(const yactfr::DataType& dt)
{
    _highlight = &dt;
    this->_redrawContent();
}

void DtExplorerView::clearHighlight()
{
    _highlight = nullptr;
    this->_redrawContent();
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

        if (const auto detailsRow = boost::get<const AbstractDtDetails *>(&row)) {
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
    this->_redrawContent();
}

void DtExplorerView::_drawScopeSubtitleRow(const Index index, const std::string& text)
{
    this->_stylist().std(*this, true);
    this->_moveAndPrint({0, this->_contentRectYFromIndex(index)}, "%s", text.c_str());
}

void DtExplorerView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();

    if (this->_rowCount() == 0) {
        this->_stylist().std(*this, true);

        const std::string msg {"Nothing to show here!"};

        this->_safeMoveAndPrint({
            this->contentRect().w / 2 - msg.size() / 2, this->contentRect().h / 2
        }, "%s", msg.c_str());
        return;
    }

    assert(this->_index() < this->_rowCount());

    for (Index index = this->_index(); index < this->_index() + this->contentRect().h; ++index) {
        if (index >= _rows.size()) {
            return;
        }

        const auto y = this->_contentRectYFromIndex(index);
        auto& row = _rows[index];

        if (const auto subRow = boost::get<_ScopeSubtitleRow>(&row)) {
            this->_moveCursor({0, y});
            this->_stylist().sectionTitle(*this);

            switch (subRow->scope) {
            case yactfr::Scope::PACKET_HEADER:
                this->_print("Packet header");
                break;

            case yactfr::Scope::PACKET_CONTEXT:
                this->_print("Packet context");
                break;

            case yactfr::Scope::EVENT_RECORD_HEADER:
                this->_print("Event record header");
                break;

            case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
                this->_print("Event record first context");
                break;

            case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
                this->_print("Event record second context");
                break;

            case yactfr::Scope::EVENT_RECORD_PAYLOAD:
                this->_print("Event record payload");
                break;
            }

            this->_print(" type:");
        } else if (const auto detailsRow = boost::get<const AbstractDtDetails *>(&row)) {
            const auto details = *detailsRow;
            bool highlighted = false;

            if (const auto dtDetails = dynamic_cast<const DtDetails *>(details)) {
                if (&dtDetails->dt() == _highlight) {
                    highlighted = true;
                    this->_stylist().stdSel(*this);

                    for (Index x = 0; x < this->contentRect().w; ++x) {
                        this->_putChar({x, y}, ' ');
                    }
                }
            }

            if (!highlighted) {
                this->_stylist().std(*this);
            }

            this->_moveCursor({2, y});
            details->renderLine(this->_window(), this->contentRect().w - 2, !highlighted);
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