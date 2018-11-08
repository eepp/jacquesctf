/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_SEARCH_CONTROLLER_HPP
#define _JACQUES_SEARCH_CONTROLLER_HPP

#include <memory>

#include "search-input-view.hpp"
#include "screen.hpp"
#include "stylist.hpp"

namespace jacques {

class SearchController
{
public:
    using LiveUpdateFunc = std::function<void (const SearchQuery&)>;

public:
    explicit SearchController(const Screen& parentScreen,
                              const Stylist& stylist);
    std::unique_ptr<const SearchQuery> startLive(const std::string& init,
                                                 const LiveUpdateFunc& liveUpdateFunc);
    void parentScreenResized(const Screen& parentScreen);

    std::unique_ptr<const SearchQuery> start(const std::string& init)
    {
        return this->startLive(init, [](const auto&) {});
    }

    std::unique_ptr<const SearchQuery> start()
    {
        return this->start(std::string {});
    }

private:
    void _tryLiveUpdate(const std::string& buf,
                        const LiveUpdateFunc& liveUpdateFunc)
    {
        const auto query = SearchParser {}.parse(buf);
        const auto startX = _searchView->rect().pos.x + 1;
        const auto startY = _searchView->rect().pos.y + 1;

        if (query) {
            curs_set(0);
            liveUpdateFunc(*query);
        }

        _searchView->redraw(true);
        _searchView->drawCurrentText(buf);
        move(startY, startX + buf.size());
        curs_set(1);
    }

    static Rectangle _viewRect(const Screen& parentScreen)
    {
        const auto& screenRect = parentScreen.rect();

        return Rectangle {{2, screenRect.h - 4}, screenRect.w - 4, 3};
    }

private:
    std::unique_ptr<SearchInputView> _searchView;
};

} // namespace jacques

#endif // _JACQUES_SEARCH_CONTROLLER_HPP
