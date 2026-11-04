/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <algorithm>

#include "dst-table-view.hpp"

namespace jacques {

DstTableView::DstTableView(const Rect& rect, const Stylist& stylist,
                           const InspectCmdState& appState) :
    TableView {rect, "Data stream types", DecorationStyle::BORDERS, stylist}
{
    this->_cols({
        _Col {"ID", 10, _COL_REQUIRED | _COL_EMPH, [this](const Index row) {
            return this->_createUIntCell((*_dsts)[row]->id());
        }},
        _Col {"Event record types", 20, _COL_REQUIRED, [this](const Index row) {
            return this->_createUIntCell((*_dsts)[row]->eventRecordTypes().size());
        }},
    });

    this->_buildRows(appState);
}

void DstTableView::_buildRows(const InspectCmdState& appState)
{
    for (auto& dsFileState : appState.dsFileStates()) {
        auto& metadata = dsFileState->metadata();
        auto& traceType = metadata.traceType();

        const auto [it, inserted] = _rows.try_emplace(&traceType);

        if (!inserted) {
            continue;
        }

        auto& dstVec = it->second;

        for (auto& dst : traceType) {
            dstVec.push_back(dst.get());
        }
    }
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
    this->_removeSel();
    this->_updateCounts();

    if (this->_rowCount() > 0) {
        this->_selectRow(0);
    }

    this->_invalidate();
}

const yactfr::DataStreamType *DstTableView::dst() const
{
    if (!_dsts) {
        return nullptr;
    }

    assert(this->_selRow());
    return (*_dsts)[*this->_selRow()];
}

void DstTableView::selectDst(const yactfr::TypeId id)
{
    if (!_dsts) {
        return;
    }

    for (Index index = 0; index < _dsts->size(); ++index) {
        auto& dst = (*_dsts)[index];

        if (dst->id() == id) {
            this->_selectRow(index);
            return;
        }
    }
}

} // namespace jacques
