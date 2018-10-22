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

#include "event-record-type-table-view.hpp"
#include "utils.hpp"

namespace jacques {

EventRecordTypeTableView::EventRecordTypeTableView(const Rectangle& rect,
                                                   const Stylist& stylist,
                                                   const State& state) :
    TableView {rect, "Event record types", DecorationStyle::BORDERS, stylist}
{
    this->_buildRows(state);
    this->_buildLogLevelNames();
    this->_setColumnDescriptions();
}

void EventRecordTypeTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescriptions();
}

void EventRecordTypeTableView::_resetRow(const std::vector<TableViewColumnDescription>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    _row.push_back(std::make_unique<TextTableViewCell>(TableViewCell::TextAlignment::LEFT));
    _row[1]->emphasized(true);
}

void EventRecordTypeTableView::_setColumnDescriptions()
{
    assert(this->contentRect().w >= 30);

    std::vector<TableViewColumnDescription> descrs {
        TableViewColumnDescription {"ID", 10},
        TableViewColumnDescription {"Name", 4},
    };

    descrs[1] = TableViewColumnDescription {
        descrs[1].title(),
        this->_contentSize(descrs.size()) - descrs[0].contentWidth()
    };

    this->_resetRow(descrs);
    this->_columnDescriptions(std::move(descrs));
}

void EventRecordTypeTableView::_buildRows(const State& state)
{
    for (auto& dsFileState : state.dataStreamFileStates()) {
        auto& metadata = dsFileState->metadata();
        auto traceType = metadata.traceType();

        for (auto& dst : *traceType) {
            auto it = _rows.find(dst.get());

            if (it != std::end(_rows)) {
                continue;
            }

            auto& ertVec = _rows[dst.get()];

            for (auto& ert : *dst) {
                ertVec.push_back(ert.get());
            }

            std::sort(std::begin(ertVec), std::end(ertVec),
                      [](const auto& ertA, const auto& ertB) {
                if (ertA->name() && ertB->name()) {
                    return *ertA->name() < *ertB->name();
                }

                return ertA->id() < ertB->id();
            });
        }
    }
}

void EventRecordTypeTableView::_drawRow(const Index index)
{
    if (!_eventRecordTypes) {
        return;
    }

    const auto ert = (*_eventRecordTypes)[index];

    static_cast<UnsignedIntTableViewCell&>(*_row[0]).value(ert->id());

    if (ert->name()) {
        _row[1]->na(false);
        static_cast<TextTableViewCell&>(*_row[1]).text(*ert->name());
    } else {
        _row[1]->na(true);
    }

    this->_drawCells(index, _row);
}

void EventRecordTypeTableView::_buildLogLevelNames()
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

bool EventRecordTypeTableView::_hasIndex(const Index index)
{
    if (!_eventRecordTypes) {
        return false;
    }

    return index < _eventRecordTypes->size();
}

void EventRecordTypeTableView::dataStreamType(const yactfr::DataStreamType& dst)
{
    const auto eventRecordTypes = &_rows[&dst];

    if (_eventRecordTypes == eventRecordTypes) {
        return;
    }

    _eventRecordTypes = eventRecordTypes;
    this->_baseIndex(0, false);
    this->_selectionIndex(0, false);
    this->_redrawContent();
}

const yactfr::EventRecordType *EventRecordTypeTableView::eventRecordType() const
{
    if (!_eventRecordTypes) {
        return nullptr;
    }

    return (*_eventRecordTypes)[this->_selectionIndex()];
}

void EventRecordTypeTableView::_selectLast()
{
    if (!_eventRecordTypes) {
        return;
    }

    this->_selectionIndex(_eventRecordTypes->size() - 1);
}

void EventRecordTypeTableView::selectEventRecordType(const std::string& pattern,
                                                     const boost::optional<SearchDirection>& direction)
{
    if (!_eventRecordTypes || _eventRecordTypes->empty()) {
        return;
    }

    Index startIndex = 0;

    if (direction) {
        assert(*direction == SearchDirection::FORWARD);

        startIndex = this->_selectionIndex() + 1;

        if (startIndex == _eventRecordTypes->size()) {
            startIndex = 0;
        }
    }

    for (Index index = startIndex; index < _eventRecordTypes->size(); ++index) {
        auto& ert = (*_eventRecordTypes)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selectionIndex(index);
            return;
        }
    }

    for (Index index = 0; index < startIndex; ++index) {
        auto& ert = (*_eventRecordTypes)[index];

        if (ert->name() && utils::globMatch(pattern, *ert->name())) {
            this->_selectionIndex(index);
            return;
        }
    }
}

void EventRecordTypeTableView::selectEventRecordType(const yactfr::TypeId id)
{
    if (!_eventRecordTypes) {
        return;
    }

    for (Index index = 0; index < _eventRecordTypes->size(); ++index) {
        auto& ert = (*_eventRecordTypes)[index];

        if (ert->id() == id) {
            this->_selectionIndex(index);
            return;
        }
    }
}

} // namespace jacques
