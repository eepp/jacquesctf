/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "pkt-table-view.hpp"
#include "../../state/msg.hpp"

namespace jacques {

PktTableView::PktTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    TableView {rect, "Packets", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_setColumnDescrs();
}

void PktTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void PktTableView::_setColumnDescrs()
{
    std::vector<TableViewColumnDescr> descrs {
        TableViewColumnDescr {"Index", 12},
        TableViewColumnDescr {"Offset", 16},
        TableViewColumnDescr {"Total length", 16},
        TableViewColumnDescr {"Content length", 16},
        TableViewColumnDescr {"Timestamp: beginning", 29},
        TableViewColumnDescr {"Timestamp: end", 29},
        TableViewColumnDescr {"Duration", 23},
        TableViewColumnDescr {"Event records", 13},
        TableViewColumnDescr {"DST ID", 6},
        TableViewColumnDescr {"DS ID", 5},
        TableViewColumnDescr {"Seq num", 8},
        TableViewColumnDescr {"Disc ER", 10},
    };

    const auto accOp = [](const auto sz, const auto& descr) {
        return sz + descr.contentWidth();
    };
    auto curSize = std::accumulate(descrs.begin(), descrs.end(), 0ULL, accOp);

    // remove columns until they all fit
    while (this->_contentSize(descrs.size()) < curSize) {
        descrs.pop_back();
        curSize = std::accumulate(descrs.begin(), descrs.end(), 0ULL, accOp);
    }

    // expand last column
    descrs.back() = TableViewColumnDescr {
        descrs.back().title(),
        this->_contentSize(descrs.size()) - std::accumulate(descrs.begin(), descrs.end() - 1, 0ULL,
                                                            accOp)
    };

    this->_resetRow(descrs);
    this->_colDescrs(std::move(descrs));
}

void PktTableView::_resetRow(const std::vector<TableViewColumnDescr>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    _row.back()->emphasized(true);
    static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));
    static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));

    if (descrs.size() >= 4) {
        _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));
    }

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<TsTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<TsTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 7) {
        _row.push_back(std::make_unique<DurationTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 8) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
        static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    }

    if (descrs.size() >= 9) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }

    if (descrs.size() >= 10) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }

    if (descrs.size() >= 11) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }

    if (descrs.size() >= 12) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }
}

void PktTableView::_drawRow(const Index row)
{
    const auto& dsf = _appState->activeDsFileState().dsFile();

    assert(row < dsf.pktCount());

    auto& entry = dsf.pktIndexEntry(row);
    const auto prevEntry = (row == 0) ? nullptr : &dsf.pktIndexEntry(row - 1);
    const auto nextEntry = (row >= dsf.pktCount() - 1) ? nullptr : &dsf.pktIndexEntry(row + 1);

    // set all cell styles to normal initially
    for (auto& cell : _row) {
        cell->style(TableViewCell::Style::NORMAL);
    }

    static_cast<UIntTableViewCell&>(*_row[0]).val(entry.natIndexInDsFile());
    static_cast<DataLenTableViewCell&>(*_row[1]).len(entry.offsetInDsFileBits());
    static_cast<DataLenTableViewCell&>(*_row[2]).len(entry.effectiveTotalLen());
    static_cast<DataLenTableViewCell&>(*_row[3]).len(entry.effectiveContentLen());

    Index at = 4;

    if (_row.size() >= at + 1) {
        if (entry.beginTs()) {
            if (prevEntry && prevEntry->endTs()) {
                if (*entry.beginTs() < *prevEntry->endTs()) {
                    _row[at]->style(TableViewCell::Style::ERROR);
                }
            }

            if (entry.endTs() && *entry.beginTs() > *entry.endTs()) {
                _row[at]->style(TableViewCell::Style::ERROR);
            }

            _row[at]->na(false);
            static_cast<TsTableViewCell&>(*_row[at]).ts(*entry.beginTs());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.endTs()) {
            if (entry.beginTs() && *entry.beginTs() > *entry.endTs()) {
                _row[at]->style(TableViewCell::Style::ERROR);
            }

            if (nextEntry && nextEntry->beginTs()) {
                if (*entry.endTs() > *nextEntry->beginTs()) {
                    _row[at]->style(TableViewCell::Style::ERROR);
                }
            }

            _row[at]->na(false);
            static_cast<TsTableViewCell&>(*_row[at]).ts(*entry.endTs());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.beginTs() && entry.endTs()) {
            _row[at]->na(false);

            auto& cell = static_cast<DurationTableViewCell&>(*_row[at]);

            cell.beginTs(*entry.beginTs());
            cell.endTs(*entry.endTs());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.erCount()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(*entry.erCount());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.dst()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(entry.dst()->id());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        if (entry.dsId()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(*entry.dsId());
        } else {
            _row[at]->na(true);
        }

        ++at;
    }

    if (_row.size() >= at + 1) {
        _row[at]->style(TableViewCell::Style::NORMAL);

        if (entry.seqNum()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(*entry.seqNum());

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

        if (entry.discErCounterSnap()) {
            _row[at]->na(false);
            static_cast<UIntTableViewCell&>(*_row[at]).val(*entry.discErCounterSnap());

            if (prevEntry && prevEntry->discErCounterSnap()) {
                const auto diff = *entry.discErCounterSnap() -
                                  *prevEntry->discErCounterSnap();

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
        if (entry.isInvalid() && cell->style() == TableViewCell::Style::NORMAL) {
            cell->style(TableViewCell::Style::ERROR);
        }
    }

    this->_drawCells(row, _row);
}

Size PktTableView::_rowCount()
{
    return _appState->activeDsFileState().dsFile().pktCount();
}

void PktTableView::tsFmtMode(const TsFmtMode tsFmtMode)
{
    static_cast<TsTableViewCell&>(*_row[4]).fmtMode(tsFmtMode);

    if (_row.size() >= 6) {
        static_cast<TsTableViewCell&>(*_row[5]).fmtMode(tsFmtMode);
    }

    if (_row.size() >= 7) {
        static_cast<DurationTableViewCell&>(*_row[6]).fmtMode(tsFmtMode);
    }

    _tsFmtMode = tsFmtMode;
    this->_redrawRows();
}

void PktTableView::dataLenFmtMode(const utils::LenFmtMode dataLenFmtMode)
{
    static_cast<DataLenTableViewCell&>(*_row[1]).fmtMode(dataLenFmtMode);
    static_cast<DataLenTableViewCell&>(*_row[2]).fmtMode(dataLenFmtMode);
    static_cast<DataLenTableViewCell&>(*_row[3]).fmtMode(dataLenFmtMode);
    _dataLenFmtMode = dataLenFmtMode;
    this->_redrawRows();
}

Index PktTableView::selPktIndex() const noexcept
{
    return this->_selRow();
}

void PktTableView::selPktIndex(const Index index)
{
    this->_selRowAndDraw(index);
}

void PktTableView::_appStateChanged(const Message msg)
{
    auto updateSel = false;

    if (msg == Message::ACTIVE_DS_FILE_CHANGED) {
        /*
         * Go back to 0 without drawing first in case there's less
         * packets than our current selection index.
         */
        this->_selRowAndDraw(0, false);
        this->_updateCounts();
        this->_redrawRows();
        updateSel = true;
    } else if (msg == Message::ACTIVE_PKT_CHANGED) {
        updateSel = true;
    }

    if (updateSel) {
        // reset selection from state
        this->_selRowAndDraw(_appState->activeDsFileState().activePktStateIndex());
    }
}

} // namespace jacques
