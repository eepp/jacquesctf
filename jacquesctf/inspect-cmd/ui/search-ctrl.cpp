/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <chrono>
#include <thread>
#include <curses.h>

#include "search-ctrl.hpp"
#include "../state/search-query.hpp"

namespace jacques {

SearchCtrl::SearchCtrl(const Screen& parentScreen, const Stylist& stylist) :
    _searchView {std::make_unique<SearchInputView>(SearchCtrl::_viewRect(parentScreen), stylist)}
{
}

std::unique_ptr<const SearchQuery> SearchCtrl::startLive(const std::string& init,
                                                         const LiveUpdateFunc& liveUpdateFunc)
{
    const auto lineLen = _searchView->contentRect().w - 2;
    std::string buf = init;

    _searchView->redraw();
    _searchView->isVisible(true);

    const auto startX = _searchView->rect().pos.x + 1;
    const auto startY = _searchView->rect().pos.y + 1;

    move(startY, startX);

    const auto prevCurs = curs_set(1);

    move(startY, startX + buf.size());
    this->_tryLiveUpdate(buf, liveUpdateFunc);
    _searchView->refresh();
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
                this->_tryLiveUpdate(buf, liveUpdateFunc);
            }
        } else if (ch == 23) {
            // ctrl+w
            buf.clear();
            this->_tryLiveUpdate(buf, liveUpdateFunc);
        } else if (ch == 4) {
            // ctrl+d
            break;
        } else if (std::isprint(ch)) {
            if (buf.size() == lineLen - 1) {
                // no space
                continue;
            }

            buf.push_back(static_cast<char>(ch));
            this->_tryLiveUpdate(buf, liveUpdateFunc);
        }

        _searchView->refresh();
        doupdate();
    }

    _searchView->isVisible(false);
    curs_set(prevCurs);

    if (!accepted || buf.empty()) {
        return nullptr;
    }

    return parseSearchQuery(buf);
}

void SearchCtrl::parentScreenResized(const Screen& parentScreen)
{
    _searchView->moveAndResize(SearchCtrl::_viewRect(parentScreen));
}

void SearchCtrl::animate(std::atomic_bool& stop) const
{
    using namespace std::chrono_literals;

    Index animIndex = 0;

    _searchView->isVisible(true);

    while (true) {
        if (stop) {
            return;
        }

        _searchView->animateBorder(animIndex);
        ++animIndex;
        _searchView->refresh(true);
        doupdate();
        std::this_thread::sleep_for(50ms);
    }

    _searchView->isVisible(false);
}

void SearchCtrl::_tryLiveUpdate(const std::string& buf, const LiveUpdateFunc& liveUpdateFunc)
{
    const auto query = parseSearchQuery(buf);
    const auto startX = _searchView->rect().pos.x + 1;
    const auto startY = _searchView->rect().pos.y + 1;

    if (query) {
        curs_set(0);
        liveUpdateFunc(*query);
    }

    _searchView->redraw(true);
    _searchView->drawCurText(buf);
    move(startY, startX + buf.size());
    curs_set(1);
}

} // namespace jacques
