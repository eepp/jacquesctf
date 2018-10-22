/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "data-stream-type-table-view.hpp"

namespace jacques {

DataStreamTypeTableView::DataStreamTypeTableView(const Rectangle& rect,
                                                 const Stylist& stylist,
                                                 const State& state) :
    TableView {rect, "Data stream types", DecorationStyle::BORDERS, stylist}
{
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    _row[0]->emphasized(true);
    this->_setColumnDescriptions();
    this->_buildRows(state);
}

void DataStreamTypeTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescriptions();
}

void DataStreamTypeTableView::_setColumnDescriptions()
{
    assert(this->contentRect().w >= 30);

    std::vector<TableViewColumnDescription> descrs {
        TableViewColumnDescription {"ID", 10},
        TableViewColumnDescription {"Event record types", 20},
    };

    descrs[1] = TableViewColumnDescription {
        descrs[1].title(),
        this->_contentSize(2) - descrs[0].contentWidth()
    };

    this->_columnDescriptions(std::move(descrs));
}

void DataStreamTypeTableView::_buildRows(const State& state)
{
    for (auto& dsFileState : state.dataStreamFileStates()) {
        auto& metadata = dsFileState->metadata();
        auto traceType = metadata.traceType();

        auto it = _rows.find(traceType.get());

        if (it != std::end(_rows)) {
            continue;
        }

        auto& dstVec = _rows[traceType.get()];

        for (auto& dst : *traceType) {
            dstVec.push_back(dst.get());
        }
    }
}

void DataStreamTypeTableView::_drawRow(const Index index)
{
    if (!_dataStreamTypes) {
        return;
    }

    const auto dst = (*_dataStreamTypes)[index];

    static_cast<UnsignedIntTableViewCell&>(*_row[0]).value(dst->id());
    static_cast<UnsignedIntTableViewCell&>(*_row[1]).value(dst->eventRecordTypes().size());
    this->_drawCells(index, _row);
}

bool DataStreamTypeTableView::_hasIndex(const Index index)
{
    if (!_dataStreamTypes) {
        return false;
    }

    return index < _dataStreamTypes->size();
}

void DataStreamTypeTableView::traceType(const yactfr::TraceType& traceType)
{
    const auto dataStreamTypes = &_rows[&traceType];

    if (_dataStreamTypes == dataStreamTypes) {
        return;
    }

    _dataStreamTypes = dataStreamTypes;
    this->_baseIndex(0, false);
    this->_selectionIndex(0, false);
    this->_redrawContent();
}

const yactfr::DataStreamType *DataStreamTypeTableView::dataStreamType() const
{
    if (!_dataStreamTypes) {
        return nullptr;
    }

    return (*_dataStreamTypes)[this->_selectionIndex()];
}

void DataStreamTypeTableView::_selectLast()
{
    if (!_dataStreamTypes) {
        return;
    }

    this->_selectionIndex(_dataStreamTypes->size() - 1);
}

void DataStreamTypeTableView::selectDataStreamType(const yactfr::TypeId id)
{
    if (!_dataStreamTypes) {
        return;
    }

    for (Index index = 0; index < _dataStreamTypes->size(); ++index) {
        auto& dst = (*_dataStreamTypes)[index];

        if (dst->id() == id) {
            this->_selectionIndex(index);
            return;
        }
    }
}

} // namespace jacques
