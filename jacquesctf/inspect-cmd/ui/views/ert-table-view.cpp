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

ErtTableView::ErtTableView(const Rect& rect, const Stylist& stylist, const State& state) :
    TableView {rect, "Event record types", DecorationStyle::BORDERS, stylist}
{
    this->_buildRows(state);
    this->_buildLogLevelNames();
    this->_setColumnDescrs();
}

void ErtTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void ErtTableView::_resetRow(const std::vector<TableViewColumnDescr>& descrs)
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

    this->_resetRow(descrs);
    this->_colDescrs(std::move(descrs));
}

void ErtTableView::_buildRows(const State& state)
{
    for (auto& dsFileState : state.dsFileStates()) {
        auto& metadata = dsFileState->metadata();

        for (auto& dst : *metadata.traceType()) {
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

void ErtTableView::_drawRow(const Index index)
{
    if (!_erts) {
        return;
    }

    const auto ert = (*_erts)[index];

    static_cast<UIntTableViewCell&>(*_row[0]).val(ert->id());

    if (ert->name()) {
        _row[1]->na(false);
        static_cast<TextTableViewCell&>(*_row[1]).text(*ert->name());
    } else {
        _row[1]->na(true);
    }

    this->_drawCells(index, _row);
}

void ErtTableView::_buildLogLevelNames()
{
    _logLevelNames = {
        "EMERGENCY",
        "ALERT",
        "CRITICAL",
        "ERROR",
        "WARNING",
        "NOTICE",
        "INFO",
        "DEBUG_SYSTEM",
        "DEBUG_PROGRAM",
        "DEBUG_PROCESS",
        "DEBUG_MODULE",
        "DEBUG_UNIT",
        "DEBUG_FUNCTION",
        "DEBUG_LINE",
        "DEBUG",
    };

    for (Index i = 0; i < _logLevelNames.size(); ++i) {
        _logLevelNames[i] += " (" + std::to_string(i) + ")";
    }
}

bool ErtTableView::_hasIndex(const Index index)
{
    if (!_erts) {
        return false;
    }

    return index < _erts->size();
}

void ErtTableView::dst(const yactfr::DataStreamType& dst)
{
    const auto erts = &_rows[&dst];

    if (_erts == erts) {
        return;
    }

    _erts = erts;
    this->_baseIndex(0, false);
    this->_selIndex(0, false);
    this->_redrawContent();
}

const yactfr::EventRecordType *ErtTableView::ert() const noexcept
{
    if (!_erts) {
        return nullptr;
    }

    return (*_erts)[this->_selIndex()];
}

void ErtTableView::_selectLast()
{
    if (!_erts) {
        return;
    }

    this->_selIndex(_erts->size() - 1);
}

void ErtTableView::selectErt(const std::string& pattern, const bool relative)
{
    if (!_erts || _erts->empty()) {
        return;
    }

    Index startIndex = 0;

    if (relative) {
        startIndex = this->_selIndex() + 1;

        if (startIndex == _erts->size()) {
            startIndex = 0;
        }
    }

    for (Index index = startIndex; index < _erts->size(); ++index) {
        auto& ert = (*_erts)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selIndex(index);
            return;
        }
    }

    for (Index index = 0; index < startIndex; ++index) {
        auto& ert = (*_erts)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selIndex(index);
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
            this->_selIndex(index);
            return;
        }
    }
}

} // namespace jacques
