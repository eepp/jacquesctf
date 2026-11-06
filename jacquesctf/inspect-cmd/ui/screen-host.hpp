/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREEN_HOST_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREEN_HOST_HPP

namespace jacques {

class Overlay;

/*
 * Interface of the main loop of the inspect command, which owns the
 * screens, as the screens see it.
 */
class ScreenHost
{
protected:
    explicit ScreenHost() noexcept = default;

public:
    virtual ~ScreenHost() = default;

    /*
     * Makes the inspect screen the current screen.
     */
    void switchToInspectScreen()
    {
        this->_switchToInspectScreen();
    }

    /*
     * Makes the packets screen the current screen.
     */
    void switchToPktsScreen()
    {
        this->_switchToPktsScreen();
    }

    /*
     * Opens `overlay` on top of the current screen and of the other
     * open overlays.
     *
     * The main loop passes the user keys to `overlay` until it closes
     * (see `Overlay`).
     *
     * `overlay` must not be open already, and must exist as long as
     * it's open.
     */
    void openOverlay(Overlay& overlay)
    {
        this->_openOverlay(overlay);
    }

    /*
     * Renders the whole terminal and updates it.
     */
    void renderFrame()
    {
        this->_renderFrame();
    }

protected:
    virtual void _switchToInspectScreen() = 0;
    virtual void _switchToPktsScreen() = 0;
    virtual void _openOverlay(Overlay& overlay) = 0;
    virtual void _renderFrame() = 0;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREEN_HOST_HPP
