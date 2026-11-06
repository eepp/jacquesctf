/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_OVERLAY_HPP
#define _JACQUES_INSPECT_COMMAND_UI_OVERLAY_HPP

#include "key.hpp"

namespace jacques {

/*
 * Base class of all overlays.
 *
 * An overlay is a modal element which the main loop of the inspect
 * command shows on top of the current screen (see
 * ScreenHost::openOverlay()), for example a search box or a
 * confirmation message.
 *
 * The main loop keeps a stack of open overlays: it passes the user
 * keys to the top overlay only, except the keys which always apply
 * (terminal resize, F10, and Ctrl+L). When handleKey() returns false,
 * the main loop hides the overlay and removes it from the stack, and
 * then calls closed().
 *
 * The main loop renders the open overlays after the current screen,
 * from the bottom of the stack to its top.
 *
 * An opaque overlay covers the whole terminal: while it's open, the
 * main loop hides everything below it (the current screen, the status
 * view, and the other open overlays).
 */
class Overlay
{
protected:
    /*
     * Builds a base overlay, initially not visible.
     */
    explicit Overlay(const bool isOpaque = false) noexcept :
        _isOpaque {isOpaque}
    {
    }

public:
    virtual ~Overlay() = default;

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    bool isOpaque() const noexcept
    {
        return _isOpaque;
    }

    /*
     * Sets the visibility of this overlay to `isVisible`.
     *
     * If the visibility changes, calls the virtual
     * _visibilityChanged() method, which must remove anything this
     * overlay shows from the terminal when it becomes hidden.
     */
    void isVisible(const bool isVisible)
    {
        if (isVisible == _isVisible) {
            return;
        }

        _isVisible = isVisible;
        this->_visibilityChanged();
    }

    bool isVisible() const noexcept
    {
        return _isVisible;
    }

    /*
     * If this overlay is visible, then renders it on top of what's
     * already rendered.
     *
     * This doesn't update the terminal itself (see term::update()).
     */
    void render()
    {
        if (_isVisible) {
            this->_render();
        }
    }

    /*
     * Handles the user key `key`, returning false to make the main
     * loop close this overlay.
     */
    bool handleKey(const Key key)
    {
        return this->_handleKey(key);
    }

    /*
     * Called by the main loop once it closed this overlay.
     *
     * This overlay may open again from there.
     */
    void closed()
    {
        this->_closed();
    }

protected:
    virtual void _render() = 0;
    virtual bool _handleKey(Key key) = 0;

    /*
     * The default implementation does nothing.
     */
    virtual void _visibilityChanged()
    {
    }

    /*
     * The default implementation does nothing.
     */
    virtual void _closed()
    {
    }

private:
    bool _isOpaque;
    bool _isVisible = false;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_OVERLAY_HPP
