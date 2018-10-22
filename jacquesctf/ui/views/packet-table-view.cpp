/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "packet-table-view.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"

namespace jacques {

PacketTableView::PacketTableView(const Rectangle& rect,
                                 const Stylist& stylist, State& state) :
    TableView {rect, "Packets", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_setColumnDescriptions();
}

void PacketTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescriptions();
}

void PacketTableView::_setColumnDescriptions()
{
    std::vector<TableViewColumnDescription> descrs {
        TableViewColumnDescription {"Index", 12},
        TableViewColumnDescription {"Offset", 16},
        TableViewColumnDescription {"Total size", 16},
        TableViewColumnDescription {"Content size", 16},
        TableViewColumnDescription {"Timestamp: beginning", 29},
        TableViewColumnDescription {"Timestamp: end", 29},
        TableViewColumnDescription {"Duration", 23},
        TableViewColumnDescription {"Event records", 13},
        TableViewColumnDescription {"DST ID", 6},
        TableViewColumnDescription {"DS ID", 5},
        TableViewColumnDescription {"Seq num", 8},
        TableViewColumnDescription {"Discard ER", 10},
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

    // expand last column
    descrs.back() = TableViewColumnDescription {
        descrs.back().title(),
        this->_contentSize(descrs.size()) -
            std::accumulate(std::begin(descrs), std::end(descrs) - 1,
                            0ULL, accOp)
    };

    this->_resetRow(descrs);
    this->_columnDescriptions(std::move(descrs));
}

void PacketTableView::_resetRow(const std::vector<TableViewColumnDescription>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    _row.back()->emphasized(true);
    static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));
    static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));

    if (descrs.size() >= 4) {
        _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));
    }

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<TimestampTableViewCell>(_tsFormatMode));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<TimestampTableViewCell>(_tsFormatMode));
    }

    if (descrs.size() >= 7) {
        _row.push_back(std::make_unique<DurationTableViewCell>(_tsFormatMode));
    }

    if (descrs.size() >= 8) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
        static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    }

    if (descrs.size() >= 9) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }

    if (descrs.size() >= 10) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }

    if (descrs.size() >= 11) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }

    if (descrs.size() >= 12) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }
}

void PacketTableView::_drawRow(const Index index)
{
    const auto& dsf = _state->activeDataStreamFileState().dataStreamFile();

    assert(index < dsf.packetCount());

    const auto& entry = dsf.packetIndexEntry(index);
    const auto prevEntry = (index == 0) ? nullptr : &dsf.packetIndexEntry(index - 1);

    static_cast<UnsignedIntTableViewCell&>(*_row[0]).value(entry.natIndexInDataStream());
    static_cast<DataSizeTableViewCell&>(*_row[1]).size(entry.offsetInDataStreamBits());
    static_cast<DataSizeTableViewCell&>(*_row[2]).size(entry.effectiveTotalSize());
    static_cast<DataSizeTableViewCell&>(*_row[3]).size(entry.effectiveContentSize());

    Index at = 4;

    if (_row.size() >= at + 1) {
        if (entry.beginningTimestamp()) {
            _row[4]->na(false);
            static_cast<TimestampTableViewCell&>(*_row[4]).ts(*entry.beginningTimestamp());
        } else {
            _row[4]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.endTimestamp()) {
            _row[at]->na(false);
            static_cast<TimestampTableViewCell&>(*_row[at]).ts(*entry.endTimestamp());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.beginningTimestamp() && entry.endTimestamp()) {
            _row[at]->na(false);

            auto& cell = static_cast<DurationTableViewCell&>(*_row[at]);

            cell.beginningTimestamp(*entry.beginningTimestamp());
            cell.endTimestamp(*entry.endTimestamp());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.eventRecordCount()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(*entry.eventRecordCount());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.dataStreamType()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(entry.dataStreamType()->id());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.dataStreamId()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(*entry.dataStreamId());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        _row[at]->style(TableViewCell::Style::NORMAL);

        if (entry.seqNum()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(*entry.seqNum());

            if (prevEntry && prevEntry->seqNum()) {
                if (*prevEntry->seqNum() != *entry.seqNum() - 1) {
                    _row[at]->style(TableViewCell::Style::WARNING);
                }
            }
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        _row[at]->style(TableViewCell::Style::NORMAL);

        if (entry.discardedEventRecordCounter()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(*entry.discardedEventRecordCounter());

            if (prevEntry && prevEntry->discardedEventRecordCounter()) {
                const auto diff = *entry.discardedEventRecordCounter() -
                                  *prevEntry->discardedEventRecordCounter();

                if (diff != 0) {
                    _row[at]->style(TableViewCell::Style::WARNING);
                }
            }
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    for (auto& cell : _row) {
        if (entry.isInvalid()) {
            cell->style(TableViewCell::Style::ERROR);
        } else {
            if (cell->style() == TableViewCell::Style::ERROR) {
                cell->style(TableViewCell::Style::NORMAL);
            }
        }
    }

    this->_drawCells(index, _row);
}

bool PacketTableView::_hasIndex(const Index index)
{
    return index < _state->activeDataStreamFileState().dataStreamFile().packetCount();
}

void PacketTableView::timestampFormatMode(const TimestampFormatMode tsFormatMode)
{
    static_cast<TimestampTableViewCell&>(*_row[4]).formatMode(tsFormatMode);

    if (_row.size() >= 6) {
        static_cast<TimestampTableViewCell&>(*_row[5]).formatMode(tsFormatMode);
    }

    if (_row.size() >= 7) {
        static_cast<DurationTableViewCell&>(*_row[6]).formatMode(tsFormatMode);
    }

    _tsFormatMode = tsFormatMode;
    this->_redrawRows();
}

void PacketTableView::dataSizeFormatMode(const utils::SizeFormatMode dataSizeFormatMode)
{
    static_cast<DataSizeTableViewCell&>(*_row[1]).formatMode(dataSizeFormatMode);
    static_cast<DataSizeTableViewCell&>(*_row[2]).formatMode(dataSizeFormatMode);
    static_cast<DataSizeTableViewCell&>(*_row[3]).formatMode(dataSizeFormatMode);
    _sizeFormatMode = dataSizeFormatMode;
    this->_redrawRows();
}

Index PacketTableView::selectedPacketIndex() const
{
    return this->_selectionIndex();
}

void PacketTableView::selectedPacketIndex(Index index)
{
    this->_selectionIndex(index);
}

void PacketTableView::_stateChanged(const Message& msg)
{
    bool updateSelection = false;

    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg)) {
        /*
         * Go back to 0 without drawing first in case there's less
         * packets than our current selection index.
         */
        this->_selectionIndex(0, false);
        this->redraw();
        updateSelection = true;
    } else if (auto sMsg = dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        updateSelection = true;
    }

    if (updateSelection) {
        // reset selection from state
        this->_selectionIndex(_state->activeDataStreamFileState().activePacketStateIndex());
    }
}

void PacketTableView::_selectLast()
{
    this->_selectionIndex(_state->activeDataStreamFileState().dataStreamFile().packetCount() - 1);
}

} // namespace jacques
