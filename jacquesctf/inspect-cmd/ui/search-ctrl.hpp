/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "key.hpp"
#include "overlay.hpp"
#include "screen-host.hpp"
#include "views/search-input-view.hpp"
#include "screens/screen.hpp"
#include "stylist.hpp"

namespace jacques {

/*
 * Search box of a screen.
 *
 * start() and startLive() open the search box as an overlay (see
 * `Overlay`) and return immediately; the main loop then passes the user
 * keys to it.
 *
 * When the user closes the search box, the search controller calls the
 * done function with the accepted query (if any).
 */
class SearchCtrl final :
    public Overlay
{
public:
    using LiveUpdateFunc = std::function<void (const SearchQuery&)>;

    /*
     * Function which the search controller calls when the user closes
     * the search box, with the accepted query, or `nullptr` if the user
     * canceled the search or if the input text is empty or invalid.
     */
    using DoneFunc = std::function<void (std::unique_ptr<const SearchQuery>)>;

    /*
     * Draws the next frame of the border animation of the search view
     * and updates the terminal.
     */
    using AnimateFunc = std::function<void ()>;

    /*
     * Function which animateWhile() calls.
     */
    using AnimatedFunc = std::function<void (const AnimateFunc&)>;

public:
    /*
     * Builds a search controller for the screen `parentScreen` which
     * `host` hosts.
     */
    explicit SearchCtrl(const Screen& parentScreen, const Stylist& stylist, ScreenHost& host);

    /*
     * Opens the search box with the initial input text `init`.
     *
     * The search controller calls `liveUpdateFunc()` each time the
     * input text becomes a valid query, and then `doneFunc()` when the
     * user closes the search box.
     */
    void startLive(std::string init, LiveUpdateFunc liveUpdateFunc, DoneFunc doneFunc);

    void start(std::string init, DoneFunc doneFunc)
    {
        this->startLive(std::move(init), [](const auto&) {}, std::move(doneFunc));
    }

    void start(DoneFunc doneFunc)
    {
        this->start(std::string {}, std::move(doneFunc));
    }

    void parentScreenResized(const Screen& parentScreen);

    /*
     * Initial text of the search input when the user starts a search
     * with the key `key`, or `std::nullopt` if `key` doesn't start a
     * search.
     *
     * The keys which start a search are:
     *
     * `/`, `g`:
     *     Empty text.
     *
     * `N`, `*`:
     *     `*` (event record type name).
     *
     * `k`:
     *     `**` (event record type ID).
     *
     * `:`:
     *     `:` (event record index).
     *
     * `P`:
     *     `#` (packet index).
     *
     * `$`:
     *     `$` (timestamp).
     */
    static std::optional<std::string> initFromKey(Key key);

    /*
     * Renders the whole terminal with the search view on top, and then
     * calls `func()` with a function which animates the border of the
     * search view, which `func()` calls periodically while it works.
     *
     * The search box must be closed.
     */
    void animateWhile(const AnimatedFunc& func);

private:
    void _render() override;
    bool _handleKey(Key key) override;
    void _visibilityChanged() override;
    void _closed() override;
    void _tryLiveUpdate();

    static Rect _viewRect(const Screen& parentScreen) noexcept
    {
        const auto& screenRect = parentScreen.rect();

        return Rect {{2, screenRect.h - 4}, screenRect.w - 4, 3};
    }

private:
    SearchInputView _searchView;
    ScreenHost *_host;

    // current input text
    std::string _buf;

    // whether or not the user accepted the input text
    bool _accepted = false;

    LiveUpdateFunc _liveUpdateFunc;
    DoneFunc _doneFunc;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
