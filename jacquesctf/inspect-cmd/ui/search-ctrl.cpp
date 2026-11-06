/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <utility>

#include "key.hpp"
#include "unicode.hpp"
#include "term.hpp"
#include "search-ctrl.hpp"
#include "inspect-common/search-query.hpp"

namespace jacques {

SearchCtrl::SearchCtrl(const Screen& parentScreen, const Stylist& stylist, ScreenHost& host) :
    _searchView {std::make_unique<SearchInputView>(SearchCtrl::_viewRect(parentScreen), stylist)},
    _host {&host}
{
}

void SearchCtrl::startLive(std::string init, LiveUpdateFunc liveUpdateFunc, DoneFunc doneFunc)
{
    assert(!this->isVisible());
    _buf = std::move(init);
    _accepted = false;
    _liveUpdateFunc = std::move(liveUpdateFunc);
    _doneFunc = std::move(doneFunc);
    this->_tryLiveUpdate();
    _host->openOverlay(*this);
}

bool SearchCtrl::_handleKey(const Key key)
{
    if (key.special() == SpecialKey::ENTER) {
        _accepted = true;
        return false;
    } else if (key.ch() == ctrlCh('D')) {
        return false;
    } else if (key.special() == SpecialKey::BACKSPACE) {
        if (!_buf.empty()) {
            unicode::popBackUtf8(_buf);
            this->_tryLiveUpdate();
        }
    } else if (key.ch() == ctrlCh('W')) {
        _buf.clear();
        this->_tryLiveUpdate();
    } else if (const auto ch = key.printableCh()) {
        auto newBuf = _buf;

        unicode::appendUtf8(newBuf, *ch);

        if (term::strWidth(newBuf) > _searchView->contentRect().w - 3) {
            // no space
            return true;
        }

        _buf = std::move(newBuf);
        this->_tryLiveUpdate();
    }

    return true;
}

void SearchCtrl::_render()
{
    _searchView->render();
    term::showCursor({
        _searchView->rect().pos.x + 1 + term::strWidth(_buf),
        _searchView->rect().pos.y + 1
    });
}

void SearchCtrl::_visibilityChanged()
{
    if (!this->isVisible()) {
        // nothing renders the search view anymore
        _searchView->detach();
        term::hideCursor();
    }
}

void SearchCtrl::_closed()
{
    auto query = _accepted && !_buf.empty() ? parseSearchQuery(_buf) : nullptr;

    // `doneFunc()` could start another search
    const auto doneFunc = std::move(_doneFunc);

    _liveUpdateFunc = nullptr;
    _doneFunc = nullptr;
    doneFunc(std::move(query));
}

std::optional<std::string> SearchCtrl::initFromKey(const Key key)
{
    const auto ch = key.ch();

    if (!ch) {
        return std::nullopt;
    }

    switch (*ch) {
    case U'/':
    case U'g':
        return "";

    case U'N':
    case U'*':
        return "*";

    case U'k':
        return "**";

    case U':':
        return ":";

    case U'P':
        return "#";

    case U'$':
        return "$";

    default:
        return std::nullopt;
    }
}

void SearchCtrl::parentScreenResized(const Screen& parentScreen)
{
    _searchView->moveAndResize(SearchCtrl::_viewRect(parentScreen));
}

void SearchCtrl::animateWhile(const AnimatedFunc& func) const
{
    assert(!this->isVisible());
    _host->renderFrame();

    /*
     * The search box is closed, therefore the main loop doesn't render
     * the search view: render it on top of the frame.
     */
    _searchView->render();
    term::update();

    /*
     * While `func()` runs, only draw the border animation of the search
     * view and update the terminal: the other views may not be up to
     * date with the application state which `func()` modifies.
     */
    Index animIndex = 0;

    func([this, &animIndex] {
        _searchView->animateBorder(animIndex);
        ++animIndex;
        term::update();
    });

    _searchView->detach();
}

void SearchCtrl::_tryLiveUpdate()
{
    const auto query = parseSearchQuery(_buf);

    if (query) {
        // the live update could render something else
        term::hideCursor();
        _liveUpdateFunc(*query);
    }

    _searchView->text(_buf);
}

} // namespace jacques
