/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "screen.hpp"

namespace jacques {

Screen::Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
               State& state) noexcept :
    _curRect {rect},
    _curCfg {&cfg},
    _curStylist {&stylist},
    _curState {&state}
{
}

void Screen::_visibilityChanged()
{
}

} // namespace jacques
