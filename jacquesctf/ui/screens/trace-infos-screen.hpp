/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_TRACE_INFOS_SCREEN_HPP
#define _JACQUES_TRACE_INFOS_SCREEN_HPP

#include "interactive.hpp"
#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "screen.hpp"
#include "trace-infos-view.hpp"

namespace jacques {

class TraceInfosScreen :
    public Screen
{
public:
    explicit TraceInfosScreen(const Rectangle& rect, const Config& cfg,
                              std::shared_ptr<const Stylist> stylist,
                              std::shared_ptr<State> state);

protected:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<TraceInfosView> _view;
};

} // namespace jacques

#endif // _JACQUES_TRACE_INFOS_SCREEN_HPP
