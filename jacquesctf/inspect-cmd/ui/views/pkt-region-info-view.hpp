/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP

#include <unordered_map>

#include "view.hpp"

namespace jacques {

class PktRegionInfoView final :
    public View
{
public:
    explicit PktRegionInfoView(const Rect& rect, const Stylist& stylist, State& state);

private:
    void _stateChanged(Message msg) override;
    void _redrawContent() override;
    void _safePrintScope(yactfr::Scope scope);
    Size _curMaxOffsetSize();
    void _setMaxDtPathSize(const Trace& trace);
    void _setMaxDtPathSizes();

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
    std::unordered_map<const Pkt *, Size> _maxOffsetSizes;
    std::unordered_map<const Trace *, Size> _maxDtPathSizes;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_REGION_INFO_VIEW_HPP
