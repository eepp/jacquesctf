/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_TRACE_INFO_SCREEN_HPP
#define _JACQUES_TRACE_INFO_SCREEN_HPP

#include "interactive.hpp"
#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "screen.hpp"
#include "trace-info-view.hpp"

namespace jacques {

class TraceInfoScreen :
    public Screen
{
public:
    explicit TraceInfoScreen(const Rectangle& rect, const Config& cfg,
                             std::shared_ptr<const Stylist> stylist,
                             std::shared_ptr<State> state);

protected:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<TraceInfoView> _view;
};

} // namespace jacques

#endif // _JACQUES_TRACE_INFO_SCREEN_HPP
