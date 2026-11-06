/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SUB_DT_EXPLORER_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SUB_DT_EXPLORER_VIEW_HPP

#include "dt-explorer-view.hpp"

namespace jacques {

class SubDtExplorerView final :
    public DtExplorerView,
    private ViewInspectCmdStateObserver
{
public:
    explicit SubDtExplorerView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);

private:
    void _appStateChanged(Message msg) override;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SUB_DT_EXPLORER_VIEW_HPP
