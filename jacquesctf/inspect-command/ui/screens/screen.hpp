/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP

#include <memory>

#include "../inspect-command.hpp"
#include "../rectangle.hpp"
#include "config.hpp"
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
     * `rect` is the screen's rectangle within the terminal screen.
     */
    explicit Screen(const Rectangle& rect, const InspectConfig& cfg,
                    const Stylist& stylist, State& state);

public:
    virtual ~Screen();

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
     * Sets the screen's visibility to `isVisible`.
     *
     * If the visibility changes, calls the virtual _visibilityChanged()
     * method so that the derived screen can set its views's visibility
     * accordingly.
     */
    void isVisible(const bool isVisible)
    {
        if (isVisible == _visible) {
            return;
        }

        _visible = isVisible;
        this->_visibilityChanged();
    }

    bool isVisible() const
    {
        return _visible;
    }

    /*
     * Handles a user key `key`.
     */
    KeyHandlingReaction handleKey(const int key)
    {
        return this->_handleKey(key);
    }

    const Rectangle& rect() const noexcept
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
     * Implementation can perform anything visual within the screen's
     * rectangle and can modify the application's state. The return
     * value indicates what to do next.
     */
    virtual KeyHandlingReaction _handleKey(int key) = 0;

    /*
     * Called when the screen needs to be resized. rect() is already
     * changed. Implementation must move/resize its views and make sure
     * nothing is drawn outside rect().
     */
    virtual void _resized() = 0;

    /*
     * Called when the screen's visibility changes (new visibility is
     * given by isVisible()). Implementation can change the visibility
     * of its views and perform any required updates before the terminal
     * screen is updated.
     *
     * Usually, when isVisible() is true, then a full redraw of all the
     * visible views is needed to overwrite the previously visible
     * screen's characters.
     */
    virtual void _visibilityChanged();

    const InspectConfig& _config() const noexcept
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
    Rectangle _curRect;
    const InspectConfig * const _curCfg;
    const Stylist * const _curStylist;
    State * const _curState;
    bool _visible = false;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_SCREEN_HPP
