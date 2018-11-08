/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "data-stream-file-table-view.hpp"
#include "message.hpp"
#include "utils.hpp"

namespace jacques {

DataStreamFileTableView::DataStreamFileTableView(const Rectangle& rect,
                                                 const Stylist& stylist,
                                                 State& state) :
    TableView {rect, "Data stream files", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_setColumnDescriptions();
}

void DataStreamFileTableView::_stateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DATA_STREAM_FILE_CHANGED) {
        this->_selectionIndex(_state->activeDataStreamFileStateIndex());
    }
}

void DataStreamFileTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescriptions();
}

void DataStreamFileTableView::_setColumnDescriptions()
{
    _row.clear();

    std::vector<TableViewColumnDescription> descrs {
        TableViewColumnDescription {"Path", 29},
        TableViewColumnDescription {"File size", 17},
        TableViewColumnDescription {"Packets", 12},
        TableViewColumnDescription {"Timestamp: beginning", 29},
        TableViewColumnDescription {"Timestamp: end", 29},
        TableViewColumnDescription {"Duration", 23},
        TableViewColumnDescription {"DST ID", 6},
        TableViewColumnDescription {"DS ID", 5},
    };

    auto accOp = [](Size sz, const TableViewColumnDescription& descr) {
        return sz + descr.contentWidth();
    };
    auto curSize = std::accumulate(std::begin(descrs), std::end(descrs),
                                   0ULL, accOp);

    // remove some columns until they all fit
    while (this->_contentSize(descrs.size()) < curSize &&
            descrs.size() > 4) {
        descrs.erase(std::end(descrs) - 1);
        curSize = std::accumulate(std::begin(descrs), std::end(descrs),
                                  0ULL, accOp);
    }

    // expand first column
    descrs.front() = TableViewColumnDescription {
        descrs.front().title(),
        this->_contentSize(descrs.size()) -
            std::accumulate(std::begin(descrs) + 1, std::end(descrs),
                            0ULL, accOp)
    };

    this->_resetRow(descrs);
    this->_columnDescriptions(std::move(descrs));
}

void DataStreamFileTableView::_resetRow(const std::vector<TableViewColumnDescription>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<PathTableViewCell>());
    _row[0]->emphasized(true);
    _row.push_back(std::make_unique<DataSizeTableViewCell>(_sizeFormatMode));
    _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    static_cast<UnsignedIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<TimestampTableViewCell>(_tsFormatMode));

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<TimestampTableViewCell>(_tsFormatMode));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<DurationTableViewCell>(_tsFormatMode));
    }

    if (descrs.size() >= 7) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }

    if (descrs.size() >= 8) {
        _row.push_back(std::make_unique<UnsignedIntTableViewCell>(TableViewCell::TextAlignment::RIGHT));
    }
}

void DataStreamFileTableView::_drawRow(const Index index)
{
    assert(index < _state->dataStreamFileStateCount());

    const auto& dsf = _state->dataStreamFileState(index).dataStreamFile();

    static_cast<PathTableViewCell&>(*_row[0]).path(dsf.path());
    static_cast<DataSizeTableViewCell&>(*_row[1]).size(dsf.fileSize());
    static_cast<UnsignedIntTableViewCell&>(*_row[2]).value(dsf.packetCount());

    const bool hasOnePacket = dsf.packetCount() > 0;
    const PacketIndexEntry *firstEntry = nullptr;
    const PacketIndexEntry *lastEntry = nullptr;

    if (hasOnePacket) {
        firstEntry = &dsf.packetIndexEntry(0);
        lastEntry = &dsf.packetIndexEntry(dsf.packetCount() - 1);
    }

    if (hasOnePacket && firstEntry->beginningTimestamp()) {
        _row[3]->na(false);
        static_cast<TimestampTableViewCell&>(*_row[3]).ts(*firstEntry->beginningTimestamp());
    } else {
        _row[3]->na(true);
    }

    Index at = 4;

    if (_row.size() >= at + 1) {
        if (hasOnePacket && lastEntry->endTimestamp()) {
            _row[at]->na(false);
            static_cast<TimestampTableViewCell&>(*_row[at]).ts(*lastEntry->endTimestamp());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePacket && firstEntry->beginningTimestamp() &&
                lastEntry->endTimestamp()) {
            _row[at]->na(false);

            auto& cell = static_cast<DurationTableViewCell&>(*_row[at]);
            cell.beginningTimestamp(*firstEntry->beginningTimestamp());
            cell.endTimestamp(*lastEntry->endTimestamp());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePacket && firstEntry->dataStreamType()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(firstEntry->dataStreamType()->id());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePacket && firstEntry->dataStreamId()) {
            _row[at]->na(false);
            static_cast<UnsignedIntTableViewCell&>(*_row[at]).value(*firstEntry->dataStreamId());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    for (auto& cell : _row) {
        if (dsf.isComplete()) {
            cell->style(TableViewCell::Style::NORMAL);
        } else {
            cell->style(TableViewCell::Style::ERROR);
        }
    }

    this->_drawCells(index, _row);
}

bool DataStreamFileTableView::_hasIndex(const Index index)
{
    return index < _state->dataStreamFileStateCount();
}

void DataStreamFileTableView::timestampFormatMode(const TimestampFormatMode tsFormatMode)
{
    static_cast<TimestampTableViewCell&>(*_row[3]).formatMode(tsFormatMode);

    if (_row.size() >= 5) {
        static_cast<TimestampTableViewCell&>(*_row[4]).formatMode(tsFormatMode);
    }

    if (_row.size() >= 6) {
        static_cast<DurationTableViewCell&>(*_row[5]).formatMode(tsFormatMode);
    }

    _tsFormatMode = tsFormatMode;
    this->_redrawRows();
}

void DataStreamFileTableView::dataSizeFormatMode(const utils::SizeFormatMode dataSizeFormatMode)
{
    static_cast<DataSizeTableViewCell&>(*_row[1]).formatMode(dataSizeFormatMode);
    _sizeFormatMode = dataSizeFormatMode;
    this->_redrawRows();
}

Index DataStreamFileTableView::selectedDataStreamFileIndex() const
{
    return this->_selectionIndex();
}

void DataStreamFileTableView::selectedDataStreamFileIndex(const Index index)
{
    this->_selectionIndex(index);
}

void DataStreamFileTableView::_selectLast()
{
    this->_selectionIndex(_state->dataStreamFileStateCount() - 1);
}

} // namespace jacques
