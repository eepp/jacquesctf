/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP

#include <memory>

#include "../inspect-cmd.hpp"
#include "../rect.hpp"
#include "cfg.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"

namespace jacques {

/*
 * Base class for all screens.
 *
 * A screen is a controller which handles user keys and changes the
 * state or specific views accordingly (although most views are
 * autonomous, in that they react to state changes).
 *
 * A screen usually owns one or more specific views. There's never more
 * than one screen displayed on the terminal at a given time. However, a
 * screen is not necessarily full screen: there can be other,
 * "permanent" views on the top, bottom, or sides.
 *
 * As a convenience, the base screen holds the stylist and state, which
 * are almost always needed by derived screens anyway.
 */
class Screen
{
protected:
    /*
     * Builds a base screen.
     *
     * `rect` is the rectangle of the screen within the terminal screen.
     */
    explicit Screen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                    State& state) noexcept;

public:
    virtual ~Screen() = default;

    /*
     * Resizes the screen.
     *
     * The derived screen has the responsibility of resizing its views
     * accordingly in the virtual _resized() method.
     */
    void resize(const Size w, const Size h)
    {
        _curRect.w = w;
        _curRect.h = h;
        this->_resized();
        this->redraw();
    }

    /*
     * Redraws the screen.
     *
     * The derived screen has the responsibility of redrawing its views
     * accordingly in the virtual _redraw() method.
     */
    void redraw()
    {
        this->_redraw();
    }

    /*
     * Sets the visibility of the screen to `isVisible`.
     *
     * If the visibility changes, calls the virtual _visibilityChanged()
     * method so that the derived screen can set the visibility of its
     * views accordingly.
     */
    void isVisible(const bool isVisible)
    {
        if (isVisible == _isVisible) {
            return;
        }

        _isVisible = isVisible;
        this->_visibilityChanged();
    }

    bool isVisible() const
    {
        return _isVisible;
    }

    /*
     * Handles a user key `key`.
     */
    KeyHandlingReaction handleKey(const int key)
    {
        return this->_handleKey(key);
    }

    const Rect& rect() const noexcept
    {
        return _curRect;
    }

protected:
    /*
     * Implementation must redraw the whole content (rect()) of the
     * screen.
     */
    virtual void _redraw() = 0;

    /*
     * Implementation can perform anything visual within the rectangle
     * of the screen and can modify the state of the application. The
     * return value indicates what to do next.
     */
    virtual KeyHandlingReaction _handleKey(int key) = 0;

    /*
     * Called when the screen needs to be resized. rect() is already
     * changed. Implementation must move/resize its views and make sure
     * nothing is drawn outside rect().
     */
    virtual void _resized() = 0;

    /*
     * Called when the visibility of the screen changes (new visibility
     * is given by isVisible()). Implementation can change the
     * visibility of its views and perform any required updates before
     * the terminal screen is updated.
     *
     * Usually, when isVisible() is true, then a full redraw of all the
     * visible views is needed to overwrite the previously visible
     * screen characters.
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

    State& _state() noexcept
    {
        return *_curState;
    }

    const State& _state() const noexcept
    {
        return *_curState;
    }

private:
    Rect _curRect;
    const InspectCfg * const _curCfg;
    const Stylist * const _curStylist;
    State * const _curState;
    bool _isVisible = false;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
