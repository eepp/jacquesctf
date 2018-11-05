/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <curses.h>

#include "search-controller.hpp"
#include "search-parser.hpp"

namespace jacques {

SearchController::SearchController(const Screen& parentScreen,
                                   const Stylist& stylist) :
    _searchView {
        std::make_unique<SearchInputView>(SearchController::_viewRect(parentScreen),
                                          stylist)
    }
{
}

std::unique_ptr<const SearchQuery> SearchController::start()
{
    const auto lineLen = _searchView->contentRect().w - 2;
    std::string buf;

    _searchView->redraw();
    _searchView->isVisible(true);

    const auto startX = _searchView->rect().pos.x + 1;
    const auto startY = _searchView->rect().pos.y + 1;

    move(startY, startX);

    const auto prevCurs = curs_set(1);

    _searchView->refresh(true);
    doupdate();

    bool accepted = false;

    while (true) {
        const auto ch = getch();

        if (ch == '\t') {
            continue;
        } else if (ch == '\n' || ch == '\r') {
            // enter
            accepted = true;
            break;
        } else if (ch == 127 || ch == 8 || ch == KEY_BACKSPACE) {
            // backspace
            if (!buf.empty()) {
                buf.pop_back();
                _searchView->drawCurrentText(buf);
                move(startY, startX + buf.size());
            }
        } else if (ch == 23) {
            // ctrl+w
            buf.clear();
            _searchView->drawCurrentText(buf);
            move(startY, startX + buf.size());
        } else if (ch == 4) {
            // ctrl+d
            break;
        } else if (std::isprint(ch)) {
            if (buf.size() == lineLen - 1) {
                // no space
                continue;
            }

            buf.push_back(static_cast<char>(ch));
            _searchView->drawCurrentText(buf);
            move(startY, startX + buf.size());
        }

        _searchView->refresh();
        doupdate();
    }

    _searchView->isVisible(false);
    curs_set(prevCurs);

    if (!accepted || buf.empty()) {
        return nullptr;
    }

    return SearchParser {}.parse(buf);
}

void SearchController::parentScreenResized(const Screen& parentScreen)
{
    _searchView->moveAndResize(SearchController::_viewRect(parentScreen));
}

} // namespace jacques
