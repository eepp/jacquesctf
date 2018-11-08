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
    explicit SearchController(const Screen& parentScreen,
                              const Stylist& stylist);
    std::unique_ptr<const SearchQuery> start(const std::string& init);
    void parentScreenResized(const Screen& parentScreen);

    std::unique_ptr<const SearchQuery> start()
    {
        return this->start(std::string {});
    }

private:
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
