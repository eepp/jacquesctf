/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "dst-table-view.hpp"

namespace jacques {

DstTableView::DstTableView(const Rect& rect, const Stylist& stylist,
                           const InspectCmdState& appState) :
    TableView {rect, "Data stream types", DecorationStyle::BORDERS, stylist}
{
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    _row[0]->emphasized(true);
    this->_setColumnDescrs();
    this->_buildRows(appState);
}

void DstTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void DstTableView::_setColumnDescrs()
{
    assert(this->contentRect().w >= 30);

    std::vector<TableViewColumnDescr> descrs {
        TableViewColumnDescr {"ID", 10},
        TableViewColumnDescr {"Event record types", 20},
    };

    descrs[1] = TableViewColumnDescr {
        descrs[1].title(),
        this->_contentSize(2) - descrs[0].contentWidth()
    };

    this->_colDescrs(std::move(descrs));
}

void DstTableView::_buildRows(const InspectCmdState& appState)
{
    for (auto& dsFileState : appState.dsFileStates()) {
        auto& metadata = dsFileState->metadata();
        auto& traceType = metadata.traceType();

        if (_rows.find(&traceType) != _rows.end()) {
            continue;
        }

        auto& dstVec = _rows[&traceType];

        for (auto& dst : traceType) {
            dstVec.push_back(dst.get());
        }
    }
}

void DstTableView::_drawRow(const Index row)
{
    if (!_dsts) {
        return;
    }

    const auto dst = (*_dsts)[row];

    static_cast<UIntTableViewCell&>(*_row[0]).val(dst->id());
    static_cast<UIntTableViewCell&>(*_row[1]).val(dst->eventRecordTypes().size());
    this->_drawCells(row, _row);
}

Size DstTableView::_rowCount()
{
    if (!_dsts) {
        return 0;
    }

    return _dsts->size();
}

void DstTableView::traceType(const yactfr::TraceType& traceType)
{
    const auto dsts = &_rows[&traceType];

    if (_dsts == dsts) {
        return;
    }

    _dsts = dsts;
    this->_selRowAndDraw(0, false);
    this->_updateCounts();
    this->_redrawRows();
}

const yactfr::DataStreamType *DstTableView::dst() const
{
    if (!_dsts) {
        return nullptr;
    }

    return (*_dsts)[this->_selRow()];
}

void DstTableView::selectDst(const yactfr::TypeId id)
{
    if (!_dsts) {
        return;
    }

    for (Index index = 0; index < _dsts->size(); ++index) {
        auto& dst = (*_dsts)[index];

        if (dst->id() == id) {
            this->_selRowAndDraw(index);
            return;
        }
    }
}

} // namespace jacques
