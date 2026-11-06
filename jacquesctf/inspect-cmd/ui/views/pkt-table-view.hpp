/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "data/data-len.hpp"

namespace jacques {

class PktTableView final :
    public TableView,
    private ViewInspectCmdStateObserver
{
public:
    explicit PktTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);
    Index selPktIndex() const noexcept;
    void selPktIndex(Index index);

    /*
     * Call this when the packet index entries of the active data
     * stream file change (for example, after analyzing all its
     * packets).
     */
    void pktIndexEntriesChanged() noexcept
    {
        this->_invalidate();
    }

protected:
    Size _rowCount() override;
    void _appStateChanged(Message msg) override;

private:
    /*
     * Column of which the cell function receives the packet index
     * entry of the row.
     *
     * Any normal cell of an invalid packet gets the error style.
     */
    _Col _createCol(std::string title, Size width, unsigned int flags,
              std::function<_Cell (const PktIndexEntry&)> func);

    const DsFile& _dsFile() const;
    const PktIndexEntry *_prevEntry(const PktIndexEntry& entry) const;
    const PktIndexEntry *_nextEntry(const PktIndexEntry& entry) const;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP
