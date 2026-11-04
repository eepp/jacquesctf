/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "../../state/inspect-cmd-state.hpp"

namespace jacques {

class ErTableView final :
    public TableView
{
public:
    explicit ErTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);

protected:
    Size _rowCount() override;
    void _appStateChanged(Message msg) override;

private:
    /*
     * Returned event record is only valid until the next call to this
     * method (see Pkt::erAtIndexInPkt()).
     */
    const Er& _er(Index row);

private:
    InspectCmdState *_appState;
    ViewInspectCmdStateObserverGuard _appStateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
