/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP

#include <memory>
#include <vector>

#include "../key.hpp"
#include "../screen-host.hpp"
#include "../rect.hpp"
#include "cfg.hpp"
#include "../stylist.hpp"
#include "../views/view.hpp"
#include "../../state/inspect-cmd-state.hpp"

namespace jacques {

/*
 * Base class for all screens.
 *
 * A screen is a controller which handles user keys and changes the
 * state or specific views accordingly (although most views are
 * autonomous, in that they react to state changes).
 *
 * A screen usually owns one or more specific views, which it adds with
 * _addView(): render() renders them, in the order they were added, and
 * hiding the screen hides them. There's never more than one screen
 * displayed on the terminal at a given time. However, a screen is not
 * necessarily full screen: there can be other, "permanent" views on the
 * top, bottom, or sides.
 *
 * At most one view of a screen has the focus (see _focus()):
 * handleKey() passes the key to this view first (see
 * View::handleKey()), and then to _handleKey() if the view didn't
 * handle it. This is how the navigation keys (Up, Down, Page Up, and so
 * on) reach the focused view.
 *
 * A screen which mostly shows a single view adds it with
 * _addMainView(): the default _resized() makes it fill the screen.
 *
 * A screen asks its host (the main loop) to switch to another screen or
 * to open an overlay (see `Overlay`) through _host().
 *
 * As a convenience, the base screen holds the stylist and state, which
 * are almost always needed by derived screens anyway.
 */
class Screen
{
protected:
    /*
     * Builds a base screen, initially not visible.
     *
     * `rect` is the rectangle of the screen within the terminal screen.
     *
     * `host` must exist as long as this screen exists.
     */
    explicit Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                    InspectCmdState& appState, ScreenHost& host) noexcept;

public:
    virtual ~Screen() = default;

    /*
     * Resizes the screen.
     *
     * The derived screen has the responsibility of moving and resizing
     * its views accordingly in the virtual _resized() method.
     */
    void resize(const Size w, const Size h)
    {
        _curRect.w = w;
        _curRect.h = h;
        this->_resized();
    }

    /*
     * Sets the visibility of the screen to `isVisible`.
     *
     * Hiding the screen immediately hides all its views (see
     * _addView()). Showing it shows its visible views on the
     * next render().
     *
     * If the visibility changes, calls the virtual
     * _visibilityChanged() method.
     */
    void isVisible(bool isVisible);

    bool isVisible() const noexcept
    {
        return _isVisible;
    }

    /*
     * If this screen is visible, then this method renders all its views
     * (see _addView()) in the order they were added, the last one
     * ending up on top.
     *
     * This doesn't update the terminal itself (see term::update()).
     */
    void render();

    /*
     * Handles a user key `key`.
     *
     * Passes `key` to the focused view first, if any (see _focus()):
     * if it handles it, calls _focusedViewHandledKey(); otherwise,
     * calls _handleKey().
     */
    void handleKey(Key key);

    const Rect& rect() const noexcept
    {
        return _curRect;
    }

protected:
    /*
     * Adds the view `view` to the views which render() renders, on top
     * of the previously added ones.
     *
     * `view` must exist as long as this screen may render.
     */
    void _addView(View& view)
    {
        _views.push_back(&view);
    }

    /*
     * Like _addView(), but also makes `view` the main view of this
     * screen: the default _resized() makes it fill rect().
     *
     * Also focuses `view` (see _focus()).
     */
    void _addMainView(View& view);

    /*
     * Focuses `view`, blurring the previously focused view, if any.
     *
     * handleKey() passes the user keys to `view` first.
     *
     * `view` must be one of the views of this screen (see _addView()).
     */
    void _focus(View& view);

    /*
     * Focused view, or `nullptr` if none (see _focus()).
     */
    View *_focusedView() const noexcept
    {
        return _theFocusedView;
    }

    /*
     * If `key` is `t` or `s`, then selects the next timestamp or data
     * length format mode of the application state (which notifies the
     * views), and returns true.
     *
     * Otherwise, returns false.
     */
    bool _handleFmtModeKey(Key key);

    /*
     * If `key` is F3 or F4, then goes to the previous or next data
     * stream file, and returns true.
     *
     * Otherwise, returns false.
     */
    bool _handleDsFileKey(Key key);

    /*
     * Implementation can perform anything visual within the rectangle
     * of the screen and can modify the state of the application.
     *
     * handleKey() only calls this if the focused view, if any, didn't
     * handle `key`.
     *
     * The default implementation doesn't handle any key.
     */
    virtual void _handleKey(Key key);

    /*
     * Called when the focused view handled the user key `key` (see
     * handleKey()).
     *
     * For example, the selection of the focused view could have
     * changed.
     */
    virtual void _focusedViewHandledKey(Key key);

    /*
     * Called when the screen needs to be resized.
     *
     * rect() is already changed.
     *
     * Implementation must move/resize its views and make sure nothing
     * is drawn outside rect().
     *
     * The default implementation makes the main view (see
     * _addMainView()), if any, fill rect().
     */
    virtual void _resized();

    /*
     * Called when the visibility of the screen changes (new visibility
     * is given by isVisible()).
     */
    virtual void _visibilityChanged();

    const InspectCfg& _config() const noexcept
    {
        return *_curCfg;
    }

    const Stylist& _stylist() const noexcept
    {
        return *_curStylist;
    }

    InspectCmdState& _appState() noexcept
    {
        return *_curAppState;
    }

    const InspectCmdState& _appState() const noexcept
    {
        return *_curAppState;
    }

    ScreenHost& _host() const noexcept
    {
        return *_curHost;
    }

private:
    Rect _curRect;
    const InspectCfg *_curCfg;
    const Stylist *_curStylist;
    InspectCmdState *_curAppState;
    ScreenHost *_curHost;
    bool _isVisible = false;
    std::vector<View *> _views;
    View *_mainView = nullptr;
    View *_theFocusedView = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
