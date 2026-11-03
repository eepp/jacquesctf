/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP

#include <memory>
#include <vector>

#include "../inspect-cmd.hpp"
#include "../key.hpp"
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
     */
    explicit Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                    InspectCmdState& appState) noexcept;

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
     */
    KeyHandlingReaction handleKey(const Key key)
    {
        return this->_handleKey(key);
    }

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
     * Implementation can perform anything visual within the rectangle
     * of the screen and can modify the state of the application.
     *
     * The return value indicates what to do next.
     */
    virtual KeyHandlingReaction _handleKey(Key key) = 0;

    /*
     * Called when the screen needs to be resized.
     *
     * rect() is already changed.
     *
     * Implementation must move/resize its views and make sure nothing
     * is drawn outside rect().
     */
    virtual void _resized() = 0;

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

private:
    Rect _curRect;
    const InspectCfg *_curCfg;
    const Stylist *_curStylist;
    InspectCmdState *_curAppState;
    bool _isVisible = false;
    std::vector<View *> _views;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
