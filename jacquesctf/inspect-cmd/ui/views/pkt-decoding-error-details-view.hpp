/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP

#include "view.hpp"

namespace jacques {

class PktDecodingErrorDetailsView final :
    public View,
    private ViewInspectCmdStateObserver
{
public:
    explicit PktDecodingErrorDetailsView(const Rect& rect, const Stylist& stylist,
                                         InspectCmdState& appState);

private:
    void _redrawContent() override;
    void _appStateChanged(Message msg) override;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_DECODING_ERROR_DETAILS_VIEW_HPP
