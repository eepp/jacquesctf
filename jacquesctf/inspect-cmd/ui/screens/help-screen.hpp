/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "screen.hpp"
#include "../views/help-view.hpp"

namespace jacques {

class HelpScreen final :
    public Screen
{
public:
    explicit HelpScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                        State& state);

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<HelpView> _view;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_HELP_SCREEN_HPP
