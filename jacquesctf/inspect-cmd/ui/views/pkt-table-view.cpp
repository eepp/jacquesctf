/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "pkt-table-view.hpp"
#include "../../state/msg.hpp"

namespace jacques {

PktTableView::PktTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    TableView {rect, "Packets", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_cols({
        this->_createCol("Index", 12, _COL_REQUIRED | _COL_EMPH,
                         [this](const PktIndexEntry& entry) {
            return this->_createUIntCell(entry.natIndexInDsFile(), true);
        }),
        this->_createCol("Offset", 16, _COL_REQUIRED, [this](const PktIndexEntry& entry) {
            return this->_createDataLenCell(entry.offsetInDsFileBits());
        }),
        this->_createCol("Total length", 16, _COL_REQUIRED, [this](const PktIndexEntry& entry) {
            return this->_createDataLenCell(entry.effectiveTotalLen());
        }),
        this->_createCol("Content length", 16, _COL_REQUIRED, [this](const PktIndexEntry& entry) {
            return this->_createDataLenCell(entry.effectiveContentLen());
        }),
        this->_createCol("Timestamp: beginning", 29, 0, [this](const PktIndexEntry& entry) {
            const auto& beginTs = entry.beginTs();

            if (!beginTs) {
                return this->_createNaCell();
            }

            auto cell = this->_createTsCell(*beginTs);
            const auto prevEntry = this->_prevEntry(entry);

            if ((prevEntry && prevEntry->endTs() && *beginTs < *prevEntry->endTs()) ||
                    (entry.endTs() && *beginTs > *entry.endTs())) {
                cell.style = _CellStyle::ERROR;
            }

            return cell;
        }),
        this->_createCol("Timestamp: end", 29, 0, [this](const PktIndexEntry& entry) {
            const auto& endTs = entry.endTs();

            if (!endTs) {
                return this->_createNaCell();
            }

            auto cell = this->_createTsCell(*endTs);
            const auto nextEntry = this->_nextEntry(entry);

            if ((entry.beginTs() && *entry.beginTs() > *endTs) ||
                    (nextEntry && nextEntry->beginTs() && *endTs > *nextEntry->beginTs())) {
                cell.style = _CellStyle::ERROR;
            }

            return cell;
        }),
        this->_createCol("Duration", 23, 0, [this](const PktIndexEntry& entry) {
            if (!entry.beginTs() || !entry.endTs()) {
                return this->_createNaCell();
            }

            return this->_createDurationCell(*entry.beginTs(), *entry.endTs());
        }),
        this->_createCol("Event records", 13, 0, [this](const PktIndexEntry& entry) {
            if (!entry.erCount()) {
                return this->_createNaCell();
            }

            return this->_createUIntCell(*entry.erCount(), true);
        }),
        this->_createCol("DST ID", 6, 0, [this](const PktIndexEntry& entry) {
            return entry.dst() ? this->_createUIntCell(entry.dst()->id()) : this->_createNaCell();
        }),
        this->_createCol("DS ID", 5, 0, [this](const PktIndexEntry& entry) {
            return entry.dsId() ? this->_createUIntCell(*entry.dsId()) : this->_createNaCell();
        }),
        this->_createCol("Seq num", 8, 0, [this](const PktIndexEntry& entry) {
            const auto& seqNum = entry.seqNum();

            if (!seqNum) {
                return this->_createNaCell();
            }

            auto cell = this->_createUIntCell(*seqNum);
            const auto prevEntry = this->_prevEntry(entry);

            if (prevEntry && prevEntry->seqNum() && *prevEntry->seqNum() != *seqNum - 1) {
                cell.style = _CellStyle::WARNING;
            }

            return cell;
        }),
        this->_createCol("Disc ER", 10, 0, [this](const PktIndexEntry& entry) {
            const auto& snap = entry.discErCounterSnap();

            if (!snap) {
                return this->_createNaCell();
            }

            auto cell = this->_createUIntCell(*snap);
            const auto prevEntry = this->_prevEntry(entry);

            if (prevEntry && prevEntry->discErCounterSnap() &&
                    *prevEntry->discErCounterSnap() != *snap) {
                cell.style = _CellStyle::WARNING;
            }

            return cell;
        }),
    });
}

PktTableView::_Col PktTableView::_createCol(std::string title, const Size width,
                                      const unsigned int flags,
                                      std::function<_Cell (const PktIndexEntry&)> func)
{
    return _Col {std::move(title), width, flags, [this, func = std::move(func)](const Index row) {
        const auto& entry = this->_dsFile().pktIndexEntry(row);
        auto cell = func(entry);

        if (entry.isInvalid() && cell.style == _CellStyle::NORMAL) {
            cell.style = _CellStyle::ERROR;
        }

        return cell;
    }};
}

const DsFile& PktTableView::_dsFile() const
{
    return _appState->activeDsFileState().dsFile();
}

const PktIndexEntry *PktTableView::_prevEntry(const PktIndexEntry& entry) const
{
    if (entry.indexInDsFile() == 0) {
        return nullptr;
    }

    return &this->_dsFile().pktIndexEntry(entry.indexInDsFile() - 1);
}

const PktIndexEntry *PktTableView::_nextEntry(const PktIndexEntry& entry) const
{
    if (entry.indexInDsFile() + 1 >= this->_dsFile().pktCount()) {
        return nullptr;
    }

    return &this->_dsFile().pktIndexEntry(entry.indexInDsFile() + 1);
}

Size PktTableView::_rowCount()
{
    return this->_dsFile().pktCount();
}

Index PktTableView::selPktIndex() const noexcept
{
    assert(this->_selRow());
    return *this->_selRow();
}

void PktTableView::selPktIndex(const Index index)
{
    this->_selectRow(index);
}

void PktTableView::_appStateChanged(const Message msg)
{
    auto updateSel = false;

    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED) {
        /*
         * Go back to 0 without drawing first in case there's less
         * packets than our current selection index.
         */
        this->_selectRow(0);
        this->_updateCounts();
        this->_invalidate();
        updateSel = true;
    } else if (msg == Message::ACTIVE_PKT_CHANGED) {
        updateSel = true;
    }

    if (updateSel) {
        // reset selection from state
        this->_selectRow(_appState->activeDsFileState().activePktStateIndex());
    }
}

} // namespace jacques
