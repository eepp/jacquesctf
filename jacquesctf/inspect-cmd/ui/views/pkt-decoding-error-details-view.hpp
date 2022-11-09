/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP

#include <string>

#include "view.hpp"
#include "data/pkt-checkpoints.hpp"

namespace jacques {

class PktDecodingErrorDetailsView final :
    public View
{
public:
    explicit PktDecodingErrorDetailsView(const Rect& rect, const Stylist& stylist,
                                         InspectCmdState& appState);

private:
    void _redrawContent() override;
    void _appStateChanged(Message msg) override;

private:
    InspectCmdState *_appState;
    ViewInspectCmdStateObserverGuard _appStateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP
