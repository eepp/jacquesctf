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

#include "abstract-data-type-details.hpp"
#include "data-type-explorer-view.hpp"
#include "data-type-details.hpp"

namespace jacques {

DataTypeExplorerView::DataTypeExplorerView(const Rectangle& rect,
                                           const Stylist& stylist) :
    ScrollView {rect, "", DecorationStyle::BORDERS, stylist}
{
}

void DataTypeExplorerView::dataStreamType(const yactfr::DataStreamType& dataStreamType)
{
    _details.pktHeader.clear();
    _details.pktContext.clear();
    _details.ertHeader.clear();
    _details.ertFirstCtx.clear();
    _rows.clear();

    const auto traceType = dataStreamType.traceType();

    if (traceType->packetHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PACKET_HEADER});
        this->_appendDetailsRow(*traceType->packetHeaderType(),
                                _details.pktHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dataStreamType.packetContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::PACKET_CONTEXT});
        this->_appendDetailsRow(*dataStreamType.packetContextType(),
                                _details.pktContext);
        _rows.push_back(_EmptyRow {});
    }

    if (dataStreamType.eventRecordHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_HEADER});
        this->_appendDetailsRow(*dataStreamType.eventRecordHeaderType(),
                                _details.ertHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dataStreamType.eventRecordFirstContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT});
        this->_appendDetailsRow(*dataStreamType.eventRecordFirstContextType(),
                                _details.ertFirstCtx);
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

    ss << "Data stream type data types: " << "(" << dataStreamType.id() << ")";
    this->_title(ss.str());
    this->redraw();
}

void DataTypeExplorerView::eventRecordType(const yactfr::EventRecordType& eventRecordType)
{
    _details.ertHeader.clear();
    _details.ertFirstCtx.clear();
    _details.ertSecondCtx.clear();
    _details.ertPayload.clear();

    auto& dst = *eventRecordType.dataStreamType();

    _rows.clear();

    if (dst.eventRecordHeaderType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_HEADER});
        this->_appendDetailsRow(*dst.eventRecordHeaderType(),
                                _details.ertHeader);
        _rows.push_back(_EmptyRow {});
    }

    if (dst.eventRecordFirstContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT});
        this->_appendDetailsRow(*dst.eventRecordFirstContextType(),
                                _details.ertFirstCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (eventRecordType.secondContextType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT});
        this->_appendDetailsRow(*eventRecordType.secondContextType(),
                                _details.ertSecondCtx);
        _rows.push_back(_EmptyRow {});
    }

    if (eventRecordType.payloadType()) {
        _rows.push_back(_ScopeSubtitleRow {yactfr::Scope::EVENT_RECORD_PAYLOAD});
        this->_appendDetailsRow(*eventRecordType.payloadType(),
                                _details.ertPayload);
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

    if (eventRecordType.name()) {
        ss << '`' << *eventRecordType.name() << "` ";
    }

    ss << "(" << eventRecordType.id() << ")";
    this->_title(ss.str());
    this->redraw();
}

void DataTypeExplorerView::singleDataType(const yactfr::DataType& dataType,
                                          const yactfr::Scope scope)
{
    _singleDataTypeDetails.clear();
    _rows.clear();
    _rows.push_back(_ScopeSubtitleRow {scope});
    this->_appendDetailsRow(dataType, _singleDataTypeDetails);
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

void DataTypeExplorerView::highlightDataType(const yactfr::DataType& dataType)
{
    _highlight = &dataType;
    this->_redrawContent();
}

void DataTypeExplorerView::clearHighlight()
{
    _highlight = nullptr;
    this->_redrawContent();
}

void DataTypeExplorerView::centerHighlight()
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

        if (const auto detailsRow = boost::get<const AbstractDataTypeDetails *>(&row)) {
            const auto details = *detailsRow;

            if (const auto dtDetails = dynamic_cast<const DataTypeDetails *>(details)) {
                if (&dtDetails->dataType() == _highlight) {
                    break;
                }
            }
        }
    }

    if (highlightedRow == this->_rowCount()) {
        // not found
        return;
    }

    long long newIndex = static_cast<long long>(highlightedRow -
                                                this->contentRect().h / 2);

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

void DataTypeExplorerView::_drawScopeSubtitleRow(const Index index,
                                                 const std::string& text)
{
    this->_stylist().std(*this, true);
    this->_moveAndPrint({0, this->_contentRectYFromIndex(index)},
                        "%s", text.c_str());
}

void DataTypeExplorerView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();

    if (this->_rowCount() == 0) {
        this->_stylist().error(*this);
        this->_moveAndPrint(this->contentRect().pos,
                            "Nothing to show here!");
        return;
    }

    assert(this->_index() < this->_rowCount());

    for (Index index = this->_index();
            index < this->_index() + this->contentRect().h; ++index) {
        if (index >= _rows.size()) {
            return;
        }

        const auto y = this->_contentRectYFromIndex(index);
        auto& row = _rows[index];

        if (const auto subRow = boost::get<_ScopeSubtitleRow>(&row)) {
            this->_moveCursor({0, y});
            this->_stylist().detailsViewSubtitle(*this);

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
        } else if (const auto detailsRow = boost::get<const AbstractDataTypeDetails *>(&row)) {
            const auto details = *detailsRow;
            bool highlighted = false;

            if (const auto dtDetails = dynamic_cast<const DataTypeDetails *>(details)) {
                if (&dtDetails->dataType() == _highlight) {
                    highlighted = true;
                    this->_stylist().stdHighlight(*this);

                    for (Index x = 0; x < this->contentRect().w; ++x) {
                        this->_putChar({x, y}, ' ');
                    }
                }
            }

            if (!highlighted) {
                this->_stylist().std(*this);
            }

            this->_moveCursor({2, y});
            details->renderLine(this->_window(), this->contentRect().w - 2,
                                !highlighted);
        }
    }
}

void DataTypeExplorerView::_appendDetailsRow(const yactfr::DataType& dataType,
                                             _Details& details)
{
    dataTypeDetailsFromDataType(dataType, this->_stylist(), details);

    for (auto& detailsUp : details) {
        _rows.push_back(detailsUp.get());
    }
}

} // namespace jacques
