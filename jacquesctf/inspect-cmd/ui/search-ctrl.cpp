/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <atomic>
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

SearchCtrl::SearchCtrl(const Screen& parentScreen, const Stylist& stylist,
                       RenderFrameFunc renderFrameFunc) :
    _searchView {std::make_unique<SearchInputView>(SearchCtrl::_viewRect(parentScreen), stylist)},
    _renderFrameFunc {std::move(renderFrameFunc)}
{
    _searchView->isVisible(false);
}

std::unique_ptr<const SearchQuery> SearchCtrl::startLive(const std::string& init,
                                                         const LiveUpdateFunc& liveUpdateFunc)
{
    const auto lineLen = _searchView->contentRect().w - 2;
    std::string buf = init;
    const auto prevCursorIsVisible = term::cursorIsVisible();

    _searchView->isVisible(true);
    this->_tryLiveUpdate(buf, liveUpdateFunc);

    auto accepted = false;

    while (true) {
        _renderFrameFunc();

        const auto key = term::readKey();

        if (key.special() == SpecialKey::ENTER) {
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

void SearchCtrl::animateWhile(const std::function<void ()>& func) const
{
    using namespace std::chrono_literals;

    /*
     * Show the search view now, while no other thread modifies the
     * application state.
     *
     * While `func` runs in another thread, only draw the border
     * animation of the search view and update the terminal: rendering
     * the other views could read the application state while `func`
     * modifies it.
     */
    _searchView->isVisible(true);
    _renderFrameFunc();

    std::atomic_bool done {false};

    std::thread thread {[&func, &done] {
        func();
        done = true;
    }};

    for (Index animIndex = 0; !done; ++animIndex) {
        {
            const std::lock_guard<std::recursive_mutex> lock {term::mutex()};

            _searchView->animateBorder(animIndex);
            term::update();
        }

        std::this_thread::sleep_for(50ms);
    }

    thread.join();
    _searchView->isVisible(false);
}

void SearchCtrl::_tryLiveUpdate(const std::string& buf, const LiveUpdateFunc& liveUpdateFunc)
{
    const auto query = parseSearchQuery(buf);

    if (query) {
        term::hideCursor();
        liveUpdateFunc(*query);
    }

    _searchView->text(buf);

    term::showCursor({
        _searchView->rect().pos.x + 1 + term::strWidth(buf),
        _searchView->rect().pos.y + 1
    });
}

} // namespace jacques
