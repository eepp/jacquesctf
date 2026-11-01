/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

#include "key.hpp"
#include "unicode.hpp"
#include "term.hpp"
#include "search-ctrl.hpp"
#include "inspect-common/search-query.hpp"

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

    const auto prevCursorIsVisible = term::cursorIsVisible();

    this->_tryLiveUpdate(buf, liveUpdateFunc);
    _searchView->refresh();
    term::update();

    bool accepted = false;

    while (true) {
        const auto key = term::readKey();

        if (key.special() == SpecialKey::TAB) {
            continue;
        } else if (key.special() == SpecialKey::ENTER) {
            accepted = true;
            break;
        } else if (key.special() == SpecialKey::BACKSPACE) {
            if (!buf.empty()) {
                unicode::popBackUtf8(buf);
                this->_tryLiveUpdate(buf, liveUpdateFunc);
            }
        } else if (key.ch() == ctrlCh('W')) {
            buf.clear();
            this->_tryLiveUpdate(buf, liveUpdateFunc);
        } else if (key.ch() == ctrlCh('D')) {
            break;
        } else if (const auto ch = key.printableCh()) {
            auto newBuf = buf;

            unicode::appendUtf8(newBuf, *ch);

            if (term::strWidth(newBuf) > lineLen - 1) {
                // no space
                continue;
            }

            buf = std::move(newBuf);
            this->_tryLiveUpdate(buf, liveUpdateFunc);
        }

        _searchView->refresh();
        term::update();
    }

    _searchView->isVisible(false);

    if (!prevCursorIsVisible) {
        term::hideCursor();
    }

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

        {
            // the search runs in another thread which can also draw
            const std::lock_guard<std::recursive_mutex> lock {term::mutex()};

            _searchView->animateBorder(animIndex);
            ++animIndex;
            _searchView->refresh();
            term::update();
        }

        std::this_thread::sleep_for(50ms);
    }

    _searchView->isVisible(false);
}

void SearchCtrl::_tryLiveUpdate(const std::string& buf, const LiveUpdateFunc& liveUpdateFunc)
{
    const auto query = parseSearchQuery(buf);

    if (query) {
        term::hideCursor();
        liveUpdateFunc(*query);
    }

    _searchView->redraw();
    _searchView->drawCurText(buf);
    term::showCursor({
        _searchView->rect().pos.x + 1 + term::strWidth(buf),
        _searchView->rect().pos.y + 1
    });
}

} // namespace jacques
