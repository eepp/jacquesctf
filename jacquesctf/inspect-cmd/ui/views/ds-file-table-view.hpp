/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP

#include "table-view.hpp"

namespace jacques {

class DsFileTableView final :
    public TableView,
    private ViewInspectCmdStateObserver
{
public:
    explicit DsFileTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);
    Index selDsFileIndex() const;
    void selDsFileIndex(Index index);

private:
    Size _rowCount() override;
    void _appStateChanged(Message msg) override;
    const DsFile& _dsFile(Index row) const;

    // first/last packet index entry of the data stream file of `row`, if any
    const PktIndexEntry *_firstEntry(Index row) const;
    const PktIndexEntry *_lastEntry(Index row) const;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP
