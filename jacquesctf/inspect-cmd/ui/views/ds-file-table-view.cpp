/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "ds-file-table-view.hpp"
#include "../../state/msg.hpp"
#include "utils.hpp"

namespace jacques {

DsFileTableView::DsFileTableView(const Rect& rect, const Stylist& stylist,
                                 InspectCmdState& appState) :
    TableView {rect, "Data stream files", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_setColumnDescrs();
}

void DsFileTableView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED) {
        this->_selRowAndDraw(_appState->activeDsFileStateIndex());
    }
}

void DsFileTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void DsFileTableView::_setColumnDescrs()
{
    _row.clear();

    std::vector<TableViewColumnDescr> descrs {
        TableViewColumnDescr {"Path", 29},
        TableViewColumnDescr {"File size", 17},
        TableViewColumnDescr {"Packets", 12},
        TableViewColumnDescr {"Timestamp: beginning", 29},
        TableViewColumnDescr {"Timestamp: end", 29},
        TableViewColumnDescr {"Duration", 23},
        TableViewColumnDescr {"DST ID", 6},
        TableViewColumnDescr {"DS ID", 5},
    };

    const auto accOp = [](const auto sz, auto& descr) {
        return sz + descr.contentWidth();
    };
    auto curSize = std::accumulate(descrs.begin(), descrs.end(), 0ULL, accOp);

    // remove some columns until they all fit
    while (this->_contentSize(descrs.size()) < curSize && descrs.size() > 4) {
        descrs.erase(descrs.end() - 1);
        curSize = std::accumulate(descrs.begin(), descrs.end(), 0ULL, accOp);
    }

    // expand first column
    descrs.front() = TableViewColumnDescr {
        descrs.front().title(),
        this->_contentSize(descrs.size()) -
            std::accumulate(descrs.begin() + 1, descrs.end(), 0ULL, accOp)
    };

    this->_resetRow(descrs);
    this->_colDescrs(std::move(descrs));
}

void DsFileTableView::_resetRow(const std::vector<TableViewColumnDescr>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<PathTableViewCell>());
    _row[0]->emphasized(true);
    _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<TsTableViewCell>(_tsFmtMode));

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<TsTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<DurationTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 7) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }

    if (descrs.size() >= 8) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }
}

void DsFileTableView::_drawRow(const Index row)
{
    assert(row < _appState->dsFileStateCount());

    const auto& dsf = _appState->dsFileState(row).dsFile();

    static_cast<PathTableViewCell&>(*_row[0]).path(dsf.path());
    _row[0]->style(dsf.hasError() ? TableViewCell::Style::ERROR : TableViewCell::Style::NORMAL);
    static_cast<DataLenTableViewCell&>(*_row[1]).len(dsf.fileLen());
    static_cast<UIntTableViewCell&>(*_row[2]).val(dsf.pktCount());

    const bool hasOnePkt = dsf.pktCount() > 0;
    const PktIndexEntry *firstEntry = nullptr;
    const PktIndexEntry *lastEntry = nullptr;

    if (hasOnePkt) {
        firstEntry = &dsf.pktIndexEntry(0);
        lastEntry = &dsf.pktIndexEntry(dsf.pktCount() - 1);
    }

    if (hasOnePkt && firstEntry->beginTs()) {
        if (lastEntry->endTs() && *firstEntry->beginTs() > *lastEntry->endTs()) {
            _row[3]->style(TableViewCell::Style::ERROR);
        }

        _row[3]->na(false);
        static_cast<TsTableViewCell&>(*_row[3]).ts(*firstEntry->beginTs());
    } else {
        _row[3]->na(true);
    }

    Index at = 4;

    if (_row.size() >= at + 1) {
        if (hasOnePkt && lastEntry->endTs()) {
            if (firstEntry->beginTs() && *firstEntry->beginTs() > *lastEntry->endTs()) {
                _row[at]->style(TableViewCell::Style::ERROR);
            }

            _row[at]->na(false);
            static_cast<TsTableViewCell&>(*_row[at]).ts(*lastEntry->endTs());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePkt && firstEntry->beginTs() && lastEntry->endTs()) {
            _row[at]->na(false);

            auto& cell = static_cast<DurationTableViewCell&>(*_row[at]);
            cell.beginTs(*firstEntry->beginTs());
            cell.endTs(*lastEntry->endTs());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePkt && firstEntry->dst()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(firstEntry->dst()->id());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (hasOnePkt && firstEntry->dsId()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(*firstEntry->dsId());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    this->_drawCells(row, _row);
}

Size DsFileTableView::_rowCount()
{
    return _appState->dsFileStateCount();
}

void DsFileTableView::tsFmtMode(const TsFmtMode tsFmtMode)
{
    static_cast<TsTableViewCell&>(*_row[3]).fmtMode(tsFmtMode);

    if (_row.size() >= 5) {
        static_cast<TsTableViewCell&>(*_row[4]).fmtMode(tsFmtMode);
    }

    if (_row.size() >= 6) {
        static_cast<DurationTableViewCell&>(*_row[5]).fmtMode(tsFmtMode);
    }

    _tsFmtMode = tsFmtMode;
    this->_redrawRows();
}

void DsFileTableView::dataLenFmtMode(const utils::LenFmtMode dataLenFmtMode)
{
    static_cast<DataLenTableViewCell&>(*_row[1]).fmtMode(dataLenFmtMode);
    _dataLenFmtMode = dataLenFmtMode;
    this->_redrawRows();
}

Index DsFileTableView::selDsFileIndex() const
{
    return this->_selRow();
}

void DsFileTableView::selDsFileIndex(const Index index)
{
    this->_selRowAndDraw(index);
}

} // namespace jacques
