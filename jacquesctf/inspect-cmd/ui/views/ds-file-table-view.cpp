/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "ds-file-table-view.hpp"
#include "../../state/msg.hpp"

namespace jacques {

DsFileTableView::DsFileTableView(const Rect& rect, const Stylist& stylist,
                                 InspectCmdState& appState) :
    TableView {rect, "Data stream files", DecorationStyle::BORDERS, stylist, appState},
    ViewInspectCmdStateObserver {appState, *this}
{
    this->_cols({
        _Col {"Path", 29, _COL_REQUIRED | _COL_LEFT | _COL_EMPH | _COL_EXPAND,
              [this](const Index row) {
            const auto& dsf = this->_dsFile(row);
            auto cell = this->_createPathCell(dsf.path(), this->_colWidth(0));

            if (dsf.hasError()) {
                cell.style = _CellStyle::ERROR;
            }

            return cell;
        }},
        _Col {"File size", 17, _COL_REQUIRED, [this](const Index row) {
            return this->_createDataLenCell(this->_dsFile(row).fileLen());
        }},
        _Col {"Packets", 12, _COL_REQUIRED, [this](const Index row) {
            return this->_createUIntCell(this->_dsFile(row).pktCount(), true);
        }},
        _Col {"Timestamp: beginning", 29, _COL_REQUIRED, [this](const Index row) {
            const auto firstEntry = this->_firstEntry(row);

            if (!firstEntry || !firstEntry->beginTs()) {
                return this->_createNaCell();
            }

            const auto& beginTs = *firstEntry->beginTs();
            const auto& endTs = this->_lastEntry(row)->endTs();
            auto cell = this->_createTsCell(beginTs);

            if (endTs && beginTs > *endTs) {
                cell.style = _CellStyle::ERROR;
            }

            return cell;
        }},
        _Col {"Timestamp: end", 29, 0, [this](const Index row) {
            const auto lastEntry = this->_lastEntry(row);

            if (!lastEntry || !lastEntry->endTs()) {
                return this->_createNaCell();
            }

            const auto& beginTs = this->_firstEntry(row)->beginTs();
            const auto& endTs = *lastEntry->endTs();
            auto cell = this->_createTsCell(endTs);

            if (beginTs && *beginTs > endTs) {
                cell.style = _CellStyle::ERROR;
            }

            return cell;
        }},
        _Col {"Duration", 23, 0, [this](const Index row) {
            const auto firstEntry = this->_firstEntry(row);

            if (!firstEntry || !firstEntry->beginTs() || !this->_lastEntry(row)->endTs()) {
                return this->_createNaCell();
            }

            return this->_createDurationCell(*firstEntry->beginTs(),
                                             *this->_lastEntry(row)->endTs());
        }},
        _Col {"DST ID", 6, 0, [this](const Index row) {
            const auto firstEntry = this->_firstEntry(row);

            if (!firstEntry || !firstEntry->dst()) {
                return this->_createNaCell();
            }

            return this->_createUIntCell(firstEntry->dst()->id());
        }},
        _Col {"DS ID", 5, 0, [this](const Index row) {
            const auto firstEntry = this->_firstEntry(row);

            if (!firstEntry || !firstEntry->dsId()) {
                return this->_createNaCell();
            }

            return this->_createUIntCell(*firstEntry->dsId());
        }},
    });
}

const DsFile& DsFileTableView::_dsFile(const Index row) const
{
    assert(row < this->_appState().dsFileStateCount());
    return this->_appState().dsFileState(row).dsFile();
}

const PktIndexEntry *DsFileTableView::_firstEntry(const Index row) const
{
    const auto& dsf = this->_dsFile(row);

    return dsf.pktCount() > 0 ? &dsf.pktIndexEntry(0) : nullptr;
}

const PktIndexEntry *DsFileTableView::_lastEntry(const Index row) const
{
    const auto& dsf = this->_dsFile(row);

    return dsf.pktCount() > 0 ? &dsf.pktIndexEntry(dsf.pktCount() - 1) : nullptr;
}

void DsFileTableView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED) {
        this->_selectRow(this->_appState().activeDsFileStateIndex());
    } else if (msg == Message::FMT_MODES_CHANGED) {
        this->_invalidate();
    }
}

Size DsFileTableView::_rowCount()
{
    return this->_appState().dsFileStateCount();
}

Index DsFileTableView::selDsFileIndex() const
{
    assert(this->_selRow());
    return *this->_selRow();
}

void DsFileTableView::selDsFileIndex(const Index index)
{
    this->_selectRow(index);
}

} // namespace jacques
