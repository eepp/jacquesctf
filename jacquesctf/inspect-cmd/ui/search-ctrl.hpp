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
#include "views/search-input-view.hpp"
#include "screens/screen.hpp"
#include "stylist.hpp"

namespace jacques {

class SearchCtrl final
{
public:
    using LiveUpdateFunc = std::function<void (const SearchQuery&)>;

    /*
     * Renders all the views of the terminal and updates it.
     */
    using RenderFrameFunc = std::function<void ()>;

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
     * Builds a search controller for the screen `parentScreen`.
     *
     * The search controller has its own event loop: it calls
     * `renderFrameFunc()` to render the terminal.
     *
     * `parentScreen` must add view() last (see Screen::_addView()) so
     * that the search view ends up on top of its other views.
     */
    explicit SearchCtrl(const Screen& parentScreen, const Stylist& stylist,
                        RenderFrameFunc renderFrameFunc);

    [[ nodiscard ]] std::unique_ptr<const SearchQuery> startLive(const std::string& init,
                                                                 const LiveUpdateFunc& liveUpdateFunc);

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
     * Search view, which the parent screen renders.
     */
    View& view() noexcept
    {
        return *_searchView;
    }

    /*
     * Shows the search view and calls `func()` with a function which
     * animates its border, which `func()` calls periodically while it
     * works, then hides the search view.
     */
    void animateWhile(const AnimatedFunc& func) const;

    [[ nodiscard ]] std::unique_ptr<const SearchQuery> start(const std::string& init)
    {
        return this->startLive(init, [](const auto&) {});
    }

    [[ nodiscard ]] std::unique_ptr<const SearchQuery> start()
    {
        return this->start(std::string {});
    }

private:
    void _tryLiveUpdate(const std::string& buf, const LiveUpdateFunc& liveUpdateFunc);

    static Rect _viewRect(const Screen& parentScreen) noexcept
    {
        const auto& screenRect = parentScreen.rect();

        return Rect {{2, screenRect.h - 4}, screenRect.w - 4, 3};
    }

private:
    std::unique_ptr<SearchInputView> _searchView;
    RenderFrameFunc _renderFrameFunc;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
