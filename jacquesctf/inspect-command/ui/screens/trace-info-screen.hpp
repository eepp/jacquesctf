/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "screen.hpp"
#include "../views/trace-info-view.hpp"

namespace jacques {

class TraceInfoScreen :
    public Screen
{
public:
    explicit TraceInfoScreen(const Rectangle& rect, const InspectConfig& cfg,
                             const Stylist& stylist, State& state);

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<TraceInfoView> _view;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_TRACE_INFO_SCREEN_HPP
