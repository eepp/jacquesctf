/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP

#include <functional>
#include <memory>

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

public:
    /*
     * Builds a search controller for the screen `parentScreen`.
     *
     * The search controller has its own event loop: it calls
     * `renderFrameFunc()` to render the terminal, which must lock
     * term::mutex() itself.
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
     * Search view, which the parent screen renders.
     */
    View& view() noexcept
    {
        return *_searchView;
    }

    /*
     * Calls `func()` in another thread, animating the border of the
     * search view until it returns.
     */
    void animateWhile(const std::function<void ()>& func) const;

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
