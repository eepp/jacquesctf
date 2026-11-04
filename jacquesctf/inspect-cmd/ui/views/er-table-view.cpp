/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "er-table-view.hpp"
#include "../../state/msg.hpp"

namespace jacques {

ErTableView::ErTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    TableView {rect, "Event records", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_cols({
        _Col {"Index", 12, _COL_REQUIRED, [this](const Index row) {
            return this->_createUIntCell(this->_er(row).natIndexInPkt(), true);
        }},
        _Col {"Packet offset", 16, _COL_REQUIRED, [this](const Index row) {
            return this->_createDataLenCell(this->_er(row).segment().offsetInPktBits());
        }},
        _Col {"Size", 14, _COL_REQUIRED, [this](const Index row) {
            const auto& len = this->_er(row).segment().len();

            return len ? this->_createDataLenCell(*len) : this->_createNaCell();
        }},
        _Col {"ERT name", 8, _COL_REQUIRED | _COL_LEFT | _COL_EMPH | _COL_EXPAND,
              [this](const Index row) {
            const auto type = this->_er(row).type();

            if (!type || !type->name()) {
                return this->_createNaCell();
            }

            return this->_createTextCell(*type->name());
        }},
        _Col {"ERT ID", 8, 0, [this](const Index row) {
            const auto type = this->_er(row).type();

            return type ? this->_createUIntCell(type->id()) : this->_createNaCell();
        }},
        _Col {"Timestamp", 29, 0, [this](const Index row) {
            const auto& ts = this->_er(row).ts();

            return ts ? this->_createTsCell(*ts) : this->_createNaCell();
        }},
        _Col {"Duration since last ER", 23, 0, [this](const Index row) {
            const auto& ts = this->_er(row).ts();

            if (row == 0 || !ts) {
                return this->_createNaCell();
            }

            // copy: getting the previous event record invalidates `ts`
            const auto curTs = *ts;
            const auto& prevTs = this->_er(row - 1).ts();

            return prevTs ? this->_createDurationCell(*prevTs, curTs) : this->_createNaCell();
        }},
    });
}

const Er& ErTableView::_er(const Index row)
{
    assert(_appState->hasActivePktState());
    return _appState->activePktState().pkt().erAtIndexInPkt(row);
}

Size ErTableView::_rowCount()
{
    if (!_appState->hasActivePktState()) {
        return 0;
    }

    return _appState->activePktState().pkt().erCount();
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

        if (const auto curEr = _appState->curEr()) {
            this->_selectRow(curEr->indexInPkt());
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

        this->centerSelRow();
    }

    this->_invalidate();
}

} // namespace jacques
