/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "screen.hpp"

namespace jacques {

Screen::Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
               InspectCmdState& appState, ScreenHost& host) noexcept :
    _curRect {rect},
    _curCfg {&cfg},
    _curStylist {&stylist},
    _curAppState {&appState},
    _curHost {&host}
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

void Screen::handleKey(const Key key)
{
    if (_theFocusedView && _theFocusedView->handleKey(key)) {
        this->_focusedViewHandledKey(key);
        return;
    }

    this->_handleKey(key);
}

void Screen::_addMainView(View& view)
{
    assert(!_mainView);
    this->_addView(view);
    _mainView = &view;
    this->_focus(view);
}

void Screen::_focus(View& view)
{
    if (&view == _theFocusedView) {
        return;
    }

    if (_theFocusedView) {
        _theFocusedView->blur();
    }

    _theFocusedView = &view;
    view.focus();
}

bool Screen::_handleFmtModeKey(const Key key)
{
    if (key.ch() == U't') {
        this->_appState().nextTsFmtMode();
    } else if (key.ch() == U's') {
        this->_appState().nextDataLenFmtMode();
    } else {
        return false;
    }

    return true;
}

bool Screen::_handleDsFileKey(const Key key)
{
    if (key.special() == SpecialKey::F3) {
        this->_appState().gotoPrevDsFile();
    } else if (key.special() == SpecialKey::F4) {
        this->_appState().gotoNextDsFile();
    } else {
        return false;
    }

    return true;
}

void Screen::_handleKey(Key)
{
}

void Screen::_focusedViewHandledKey(Key)
{
}

void Screen::_resized()
{
    if (_mainView) {
        _mainView->moveAndResize(_curRect);
    }
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
