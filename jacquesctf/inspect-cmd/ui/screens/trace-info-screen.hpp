/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "screen.hpp"
#include "../views/trace-info-view.hpp"

namespace jacques {

class TraceInfoScreen final :
    public Screen
{
public:
    explicit TraceInfoScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                             ScreenHost& host);

private:
    void _handleKey(Key key) override;

private:
    std::unique_ptr<TraceInfoView> _view;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP
