/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <numeric>

#include "er-table-view.hpp"
#include "../../state/msg.hpp"

namespace jacques {

ErTableView::ErTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    TableView {rect, "Event records", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_setColumnDescrs();
}

void ErTableView::_resized()
{
    TableView::_resized();
    this->_setColumnDescrs();
}

void ErTableView::_resetRow(const std::vector<TableViewColumnDescr>& descrs)
{
    _row.clear();
    _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));
    static_cast<UIntTableViewCell&>(*_row.back()).sep(true);
    _row.push_back(std::make_unique<DataLenTableViewCell>(_dataLenFmtMode));

    if (descrs.size() >= 4) {
        _row.push_back(std::make_unique<TextTableViewCell>(TableViewCell::TextAlign::LEFT));
        _row.back()->emphasized(true);
    }

    if (descrs.size() >= 5) {
        _row.push_back(std::make_unique<UIntTableViewCell>(TableViewCell::TextAlign::RIGHT));
    }

    if (descrs.size() >= 6) {
        _row.push_back(std::make_unique<TsTableViewCell>(_tsFmtMode));
    }

    if (descrs.size() >= 7) {
        _row.push_back(std::make_unique<DurationTableViewCell>(_tsFmtMode));
    }
}

void ErTableView::_setColumnDescrs()
{
    std::vector<TableViewColumnDescr> descrs {
        TableViewColumnDescr {"Index", 12},
        TableViewColumnDescr {"Packet offset", 16},
        TableViewColumnDescr {"Size", 14},
        TableViewColumnDescr {"ERT name", 8},
        TableViewColumnDescr {"ERT ID", 8},
        TableViewColumnDescr {"Timestamp", 29},
        TableViewColumnDescr {"Duration since last ER", 23},
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

    // expand name column
    auto totWidth = descrs[0].contentWidth() + descrs[1].contentWidth() + descrs[2].contentWidth();

    if (descrs.size() >= 5) {
        totWidth += descrs[4].contentWidth();
    }

    if (descrs.size() >= 6) {
        totWidth += descrs[5].contentWidth();
    }

    if (descrs.size() >= 7) {
        totWidth += descrs[6].contentWidth();
    }

    descrs[3] = TableViewColumnDescr {
        descrs[3].title(),
        this->_contentSize(descrs.size()) - totWidth
    };

    this->_resetRow(descrs);
    this->_colDescrs(std::move(descrs));
}

void ErTableView::_drawRow(const Index row)
{
    assert(_appState->hasActivePktState());

    auto& er = _appState->activePktState().pkt().erAtIndexInPkt(row);

    static_cast<UIntTableViewCell&>(*_row[0]).val(er.natIndexInPkt());
    static_cast<DataLenTableViewCell&>(*_row[1]).len(er.segment().offsetInPktBits());

    if (er.segment().len()) {
        _row[2]->na(false);
        static_cast<DataLenTableViewCell&>(*_row[2]).len(*er.segment().len());
    } else {
        _row[2]->na(true);
    }

    if (_row.size() >= 4) {
        if (er.type() && er.type()->name()) {
            _row[3]->na(false);
            static_cast<TextTableViewCell&>(*_row[3]).text(*er.type()->name());
        } else {
            _row[3]->na(true);
        }
    }

    if (_row.size() >= 5) {
        if (er.type()) {
            _row[4]->na(false);
            static_cast<UIntTableViewCell&>(*_row[4]).val(er.type()->id());
        } else {
            _row[4]->na(true);
        }
    }

    if (_row.size() >= 6) {
        if (er.ts()) {
            _row[5]->na(false);
            static_cast<TsTableViewCell&>(*_row[5]).ts(*er.ts());
        } else {
            _row[5]->na(true);
        }
    }

    if (_row.size() >= 7) {
        auto& cell = static_cast<DurationTableViewCell&>(*_row[6]);

        if (row == 0 || !er.ts()) {
            cell.na(true);
        } else {
            const auto curTs = *er.ts();
            auto& prevEventRecord = _appState->activePktState().pkt().erAtIndexInPkt(row - 1);

            if (!prevEventRecord.ts()) {
                cell.na(true);
            } else {
                cell.na(false);
                cell.beginTs(*prevEventRecord.ts());
                cell.endTs(curTs);
            }
        }
    }

    this->_drawCells(row, _row);
}

Size ErTableView::_rowCount()
{
    if (!_appState->hasActivePktState()) {
        return 0;
    }

    return _appState->activePktState().pkt().erCount();
}

void ErTableView::tsFmtMode(const TsFmtMode tsFmtMode)
{
    if (_row.size() >= 6) {
        static_cast<TsTableViewCell&>(*_row[5]).fmtMode(tsFmtMode);
    }

    if (_row.size() >= 7) {
        static_cast<DurationTableViewCell&>(*_row[6]).fmtMode(tsFmtMode);
    }

    _tsFmtMode = tsFmtMode;
    this->_redrawRows();
}

void ErTableView::dataLenFmtMode(const utils::LenFmtMode dataLenFmtMode)
{
    static_cast<DataLenTableViewCell&>(*_row[1]).fmtMode(dataLenFmtMode);
    static_cast<DataLenTableViewCell&>(*_row[2]).fmtMode(dataLenFmtMode);
    _dataLenFmtMode = dataLenFmtMode;
    this->_redrawRows();
}

void ErTableView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED || msg == Message::ACTIVE_PKT_CHANGED) {
        /*
         * Remove selection in case there's less event records than our
         * current selection index.
         */
        this->_removeSel();
        this->_updateCounts();
    }

    if (_appState->hasActivePktState() && _appState->activePktState().pkt().erCount() > 0) {
        assert(_appState->activePktState().pkt().firstEr());
        assert(_appState->activePktState().pkt().lastEr());

        const auto curEr = _appState->curEr();

        if (curEr) {
            this->_selRowAndDraw(curEr->indexInPkt(), false);
        } else {
            const auto& indexEntry = _appState->activePktState().pktIndexEntry();
            const auto offsetInPktBits = _appState->curOffsetInPktBits();

            // convenience for regions outside the event record block
            if (indexEntry.preambleLen() && offsetInPktBits < indexEntry.preambleLen()->bits()) {
                this->_showFirstPage();
            } else if (offsetInPktBits >=
                    _appState->activePktState().pkt().lastEr()->segment().endOffsetInPktBits()) {
                this->_showLastPage();
            }

            this->_removeSel();
        }

        this->centerSelRow(false);
    }

    this->_redrawRows();
}

} // namespace jacques
