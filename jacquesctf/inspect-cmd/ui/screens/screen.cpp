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

void Screen::isVisible(const bool isVisible)
{
    if (isVisible == _isVisible) {
        return;
    }

    _isVisible = isVisible;

    if (!isVisible) {
        for (const auto view : _views) {
            view->detach();
        }
    }

    this->_visibilityChanged();
}

void Screen::render()
{
    if (!_isVisible) {
        return;
    }

    for (const auto view : _views) {
        view->render();
    }
}

void Screen::_visibilityChanged()
{
}

} // namespace jacques
