/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP

#include <memory>
#include <atomic>

#include "views/search-input-view.hpp"
#include "screens/screen.hpp"
#include "stylist.hpp"

namespace jacques {

class SearchCtrl final
{
public:
    using LiveUpdateFunc = std::function<void (const SearchQuery&)>;

public:
    explicit SearchCtrl(const Screen& parentScreen, const Stylist& stylist);

    std::unique_ptr<const SearchQuery> startLive(const std::string& init,
                                                 const LiveUpdateFunc& liveUpdateFunc);

    void parentScreenResized(const Screen& parentScreen);
    void animate(std::atomic_bool& stop) const;

    std::unique_ptr<const SearchQuery> start(const std::string& init)
    {
        return this->startLive(init, [](const auto&) {});
    }

    std::unique_ptr<const SearchQuery> start()
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
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SEARCH_CTRL_HPP
