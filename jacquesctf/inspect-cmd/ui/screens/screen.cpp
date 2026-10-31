/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "screen.hpp"

namespace jacques {

Screen::Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
               InspectCmdState& appState) noexcept :
    _curRect {rect},
    _curCfg {&cfg},
    _curStylist {&stylist},
    _curAppState {&appState}
{
}

void Screen::_visibilityChanged()
{
}

} // namespace jacques
