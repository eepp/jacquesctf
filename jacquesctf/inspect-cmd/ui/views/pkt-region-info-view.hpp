/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP

#include <unordered_map>

#include "view.hpp"

namespace jacques {

class PktRegionInfoView final :
    public View,
    private ViewInspectCmdStateObserver
{
public:
    explicit PktRegionInfoView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);

private:
    void _appStateChanged(Message msg) override;
    void _redrawContent() override;
    Size _curMaxOffsetSize();
    void _setMaxDtPathSize(const Trace& trace);
    void _setMaxDtPathSizes();

private:
    std::unordered_map<const Pkt *, Size> _maxOffsetSizes;
    std::unordered_map<const Trace *, Size> _maxDtPathSizes;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP
