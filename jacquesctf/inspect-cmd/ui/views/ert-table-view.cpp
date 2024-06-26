/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <sstream>
#include <string>

#include "ert-table-view.hpp"
#include "utils.hpp"

namespace jacques {

ErtTableView::ErtTableView(const Rect& rect, const Stylist& stylist, const InspectCmdState& appState) :
    TableView {rect, "Event record types", DecorationStyle::BORDERS, stylist}
{
    this->_buildRows(appState);
    this->_setColumnDescrs();
}

void ErtTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void ErtTableView::_resetRow()
{
    _row.clear();
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    _row.push_back(std::make_unique<TextTableViewCell>(TableViewCell::TextAlign::LEFT));
    _row[1]->emphasized(true);
}

void ErtTableView::_setColumnDescrs()
{
    assert(this->contentRect().w >= 30);

    std::vector<TableViewColumnDescr> descrs {
        TableViewColumnDescr {"ID", 10},
        TableViewColumnDescr {"Name", 4},
    };

    descrs[1] = TableViewColumnDescr {
        descrs[1].title(),
        this->_contentSize(descrs.size()) - descrs[0].contentWidth()
    };

    this->_resetRow();
    this->_colDescrs(std::move(descrs));
}

void ErtTableView::_buildRows(const InspectCmdState& appState)
{
    for (auto& dsFileState : appState.dsFileStates()) {
        auto& metadata = dsFileState->metadata();

        for (auto& dst : metadata.traceType()) {
            const auto it = _rows.find(dst.get());

            if (it != _rows.end()) {
                continue;
            }

            auto& ertVec = _rows[dst.get()];

            for (auto& ert : *dst) {
                ertVec.push_back(ert.get());
            }

            std::sort(ertVec.begin(), ertVec.end(), [](auto& ertA, auto& ertB) {
                if (ertA->name() && ertB->name()) {
                    return *ertA->name() < *ertB->name();
                }

                return ertA->id() < ertB->id();
            });
        }
    }
}

void ErtTableView::_drawRow(const Index row)
{
    if (!_erts) {
        return;
    }

    const auto ert = (*_erts)[row];

    static_cast<UIntTableViewCell&>(*_row[0]).val(ert->id());

    if (ert->name()) {
        _row[1]->na(false);
        static_cast<TextTableViewCell&>(*_row[1]).text(*ert->name());
    } else {
        _row[1]->na(true);
    }

    this->_drawCells(row, _row);
}

Size ErtTableView::_rowCount()
{
    if (!_erts) {
        return 0;
    }

    return _erts->size();
}

void ErtTableView::dst(const yactfr::DataStreamType& dst)
{
    const auto erts = &_rows[&dst];

    if (_erts == erts) {
        return;
    }

    _erts = erts;
    this->_updateCounts();
    this->_selRowAndDraw(0, false);
    this->_redrawRows();
}

const yactfr::EventRecordType *ErtTableView::ert() const noexcept
{
    if (!_erts) {
        return nullptr;
    }

    assert(this->_selRow());
    return (*_erts)[*this->_selRow()];
}

void ErtTableView::selectErt(const std::string& pattern, const bool relative)
{
    if (!_erts || _erts->empty()) {
        return;
    }

    Index startIndex = 0;

    if (relative) {
        assert(this->_selRow());
        startIndex = *this->_selRow() + 1;

        if (startIndex == _erts->size()) {
            startIndex = 0;
        }
    }

    for (Index index = startIndex; index < _erts->size(); ++index) {
        auto& ert = (*_erts)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selRowAndDraw(index);
            return;
        }
    }

    for (Index index = 0; index < startIndex; ++index) {
        auto& ert = (*_erts)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selRowAndDraw(index);
            return;
        }
    }
}

void ErtTableView::selectErt(const yactfr::TypeId id)
{
    if (!_erts) {
        return;
    }

    for (Index index = 0; index < _erts->size(); ++index) {
        auto& ert = (*_erts)[index];

        if (ert->id() == id) {
            this->_selRowAndDraw(index);
            return;
        }
    }
}

} // namespace jacques
