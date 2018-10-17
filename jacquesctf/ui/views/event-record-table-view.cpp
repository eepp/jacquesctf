/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "event-record-table-view.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"
#include "cur-offset-in-packet-changed-message.hpp"

namespace jacques {

EventRecordTableView::EventRecordTableView(const Rectangle& rect,
                                           std::shared_ptr<const Stylist> stylist,
                                           std::shared_ptr<State> state) :
    TableView {rect, "Event records", DecorationStyle::BORDERS, stylist},
    _state {state},
    _stateObserverGuard {*state, *this}
{
    this->_setColumnDescriptions();
}

void EventRecordTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescriptions();
}

void EventRecordTableView::_resetRow(const std::vector<TableViewColumnDescription>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));
    static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));

    if (descrs.size() >= 4) {
        _row.push_back(std::make_unique<TextTableViewCell>(TableViewCell::TextAlignment::LEFT));
        _row.back()->emphasized(true);
    }

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<TimestampTableViewCell>(_tsFormatMode));
    }
}

void EventRecordTableView::_setColumnDescriptions()
{
    std::vector<TableViewColumnDescription> descrs {
        TableViewColumnDescription {"Index", 12},
        TableViewColumnDescription {"Packet offset", 16},
        TableViewColumnDescription {"Size", 14},
        TableViewColumnDescription {"ERT name", 8},
        TableViewColumnDescription {"ERT ID", 8},
        TableViewColumnDescription {"Timestamp: first", 29},
    };

    const auto accOp = [](Size sz, const TableViewColumnDescription& descr) {
        return sz + descr.contentWidth();
    };
    auto curSize = std::accumulate(std::begin(descrs), std::end(descrs),
                                   0ULL, accOp);

    // remove columns until they all fit
    while (this->_contentSize(descrs.size()) < curSize) {
        descrs.pop_back();
        curSize = std::accumulate(std::begin(descrs), std::end(descrs),
                                  0ULL, accOp);
    }

    // expand name column
    auto totWidth = descrs[0].contentWidth() + descrs[1].contentWidth() +
                    descrs[2].contentWidth();

    if (descrs.size() >= 5) {
        totWidth += descrs[4].contentWidth();
    }

    if (descrs.size() >= 6) {
        totWidth += descrs[5].contentWidth();
    }

    descrs[3] = TableViewColumnDescription {
        descrs[3].title(),
        this->_contentSize(descrs.size()) - totWidth
    };

    this->_resetRow(descrs);
    this->_columnDescriptions(std::move(descrs));
}

void EventRecordTableView::_drawRow(const Index index)
{
    assert(_state->hasActivePacketState());

    auto& eventRecord = _state->activePacketState().packet().eventRecordAtIndexInPacket(index);

    static_cast<UnsignedIntTableViewCell&>(*_row[0]).value(eventRecord.natIndexInPacket());
    static_cast<DataSizeTableViewCell&>(*_row[1]).size(eventRecord.segment().offsetInPacketBits());
    static_cast<DataSizeTableViewCell&>(*_row[2]).size(eventRecord.segment().size());

    if (_row.size() >= 4) {
        if (eventRecord.type().name()) {
            _row[3]->na(false);
            static_cast<TextTableViewCell&>(*_row[3]).text(*eventRecord.type().name());
        } else {
            _row[3]->na(true);
        }
    }

    if (_row.size() >= 5) {
        static_cast<UnsignedIntTableViewCell&>(*_row[4]).value(eventRecord.type().id());
    }

    if (_row.size() >= 6) {
        if (eventRecord.firstTimestamp()) {
            _row[5]->na(false);
            static_cast<TimestampTableViewCell&>(*_row[5]).ts(*eventRecord.firstTimestamp());
        } else {
            _row[5]->na(true);
        }
    }

    this->_drawCells(index, _row);
}

bool EventRecordTableView::_hasIndex(const Index index)
{
    if (!_state->hasActivePacketState()) {
        return false;
    }

    return index < _state->activePacketState().packet().eventRecordCount();
}

void EventRecordTableView::timestampFormatMode(const TimestampFormatMode tsFormatMode)
{
    static_cast<TimestampTableViewCell&>(*_row[5]).formatMode(tsFormatMode);
    _tsFormatMode = tsFormatMode;
    this->_redrawRows();
}

void EventRecordTableView::dataSizeFormatMode(const utils::SizeFormatMode dsFormatMode)
{
    static_cast<DataSizeTableViewCell&>(*_row[1]).formatMode(dsFormatMode);
    static_cast<DataSizeTableViewCell&>(*_row[2]).formatMode(dsFormatMode);
    _sizeFormatMode = dsFormatMode;
    this->_redrawRows();
}

void EventRecordTableView::_selectLast()
{
    if (!_state->hasActivePacketState()) {
        return;
    }

    this->_selectionIndex(_state->activePacketState().packet().eventRecordCount() - 1);
}

void EventRecordTableView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg) ||
            dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        /*
         * Go back to 0 without drawing first in case there's less event
         * records than our current selection index.
         */
        this->_selectionIndex(0, false);
        this->_isSelectionHighlightEnabled(false, false);
    }

    if (_state->hasActivePacketState() &&
            _state->activePacketState().packet().eventRecordCount() > 0) {
        assert(_state->activePacketState().packet().firstEventRecord());
        assert(_state->activePacketState().packet().lastEventRecord());

        auto curEventRecord = _state->currentEventRecord();

        if (curEventRecord) {
            this->_isSelectionHighlightEnabled(true, false);
            this->_selectionIndex(curEventRecord->indexInPacket(), false);
        } else {
            const auto& indexEntry = _state->activePacketState().packetIndexEntry();
            const auto offsetInPacketBits = _state->curOffsetInPacketBits();

            // convenience for regions outside the event record block
            if (indexEntry.preambleSize() && offsetInPacketBits <
                                             indexEntry.preambleSize()->bits()) {
                this->_selectionIndex(0, false);
            } else if (offsetInPacketBits >=
                    _state->activePacketState().packet().lastEventRecord()->segment().endOffsetInPacketBits()) {
                this->_selectionIndex(_state->activePacketState().packet().lastEventRecord()->indexInPacket(),
                                      false);
            }

            this->_isSelectionHighlightEnabled(false, false);
        }

        this->centerSelectedRow(false);
    }

    this->_redrawRows();
}

} // namespace jacques
