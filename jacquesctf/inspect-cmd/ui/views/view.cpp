/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <curses.h>

#include "view.hpp"
#include "../stylist.hpp"
#include "utils.hpp"

namespace jacques {

View::View(const Rect& rect, const std::string& title, const DecorationStyle decoStyle,
           const Stylist& stylist) :
    _rect {rect},
    _theTitle {utils::escapeStr(title)},
    _decoStyle {decoStyle},
    _theStylist {&stylist},
    _lineBuf(2048)
{
    this->_setContentRect();
    _curWindow = newwin(rect.h, rect.w, rect.pos.y, rect.pos.x);
    assert(_curWindow);
    this->redraw();
}

View::~View()
{
    delwin(_curWindow);
}

void View::_setContentRect()
{
    const auto hasBorders = _decoStyle != DecorationStyle::BORDERLESS;

    _contentRect = {
        Point {hasBorders ? 1ULL : 0, hasBorders ? 1ULL : 0},
        hasBorders ? _rect.w - 2 : _rect.w,
        hasBorders ? _rect.h - 2 : _rect.h
    };
}

void View::refresh(const bool touch) const
{
    if (!_isVisible) {
        return;
    }

    if (touch) {
        this->touch();
    }

    const auto ret = wnoutrefresh(_curWindow);

    assert(ret == OK);
    static_cast<void>(ret);
}

void View::redraw(const bool touch)
{
    _theStylist->std(*this);
    werase(_curWindow);
    this->_decorate();
    this->_redrawContent();
    this->refresh(touch);
}

void View::touch() const
{
    touchwin(_curWindow);
}

void View::moveAndResize(const Rect& rect)
{
    _theStylist->std(*this);
    werase(_curWindow);
    this->refresh();
    wresize(_curWindow, 1, 1);
    mvwin(_curWindow, rect.pos.y, rect.pos.x);
    wresize(_curWindow, rect.h, rect.w);
    _rect = rect;
    this->_setContentRect();
    this->_resized();
    this->redraw();
}

void View::_appStateChanged(Message)
{
}

void View::_redrawContent()
{
}

void View::_resized()
{
}

void View::focus()
{
    _hasFocusMemb = true;
    this->_decorate();
}

void View::blur()
{
    _hasFocusMemb = false;
    this->_decorate();
}

void View::_decorate() const
{
    if (_decoStyle == DecorationStyle::BORDERLESS) {
        return;
    }

    _theStylist->viewBorder(*this, _hasFocusMemb, _decoStyle == DecorationStyle::BORDERS_EMPHASIZED);
    box(_curWindow, 0, 0);

    const auto maxWidth = _rect.w - 4;
    const auto title = utils::call([this, maxWidth] {
        if (_theTitle.size() > maxWidth) {
            return _theTitle.substr(0, maxWidth - 3) + "...";
        } else {
            return _theTitle;
        }
    });

    _theStylist->viewTitle(*this, _hasFocusMemb, _decoStyle == DecorationStyle::BORDERS_EMPHASIZED);

    const auto ret = mvwprintw(_curWindow, 0, 2, "%s", title.c_str());

    assert(ret == OK);
    static_cast<void>(ret);

    static auto moreStr = " +++ ";
    const auto moreX = _rect.w - 7;

    if (_moreTop) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _theStylist->viewHasMore(*this);
        mvwprintw(_curWindow, 0, moreX, "%s", moreStr);
    }

    if (_moreBottom) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _theStylist->viewHasMore(*this);
        mvwprintw(_curWindow, _rect.h - 1, moreX, "%s", moreStr);
    }
}

void View::_clearContent() const
{
    _theStylist->std(*this);

    for (Index y = 0; y < this->contentRect().h; ++y) {
        this->_putChar({0, y}, ' ');

        for (Index x = 1; x < this->contentRect().w; ++x) {
            this->_appendChar(' ');
        }
    }
}

void View::_clearRect() const
{
    for (Index y = 0; y < _rect.h; ++y) {
        mvwaddch(_curWindow, y, 0, ' ');

        for (Index x = 1; x < _rect.w; ++x) {
            this->_appendChar(' ');
        }
    }
}

int View::_print(const char * const fmt, ...) const
{
    va_list args;

    va_start(args, fmt);

    const auto ret = this->_vPrint(fmt, args);

    va_end(args);
    return ret;
}

int View::_safePrint(const char * const fmt, ...) const
{
    va_list args;

    va_start(args, fmt);

    const auto ret = this->_vSafePrint(fmt, args);

    va_end(args);
    return ret;
}

void View::_moveCursor(const Point& contentPos) const
{
    const auto x = _contentRect.pos.x + contentPos.x;
    const auto y = _contentRect.pos.y + contentPos.y;

    static_cast<void>(wmove(_curWindow, y, x));
}

int View::_safeMoveAndPrint(const Point& contentPos, const char * const fmt, ...) const
{
    this->_moveCursor(contentPos);

    va_list args;

    va_start(args, fmt);

    const auto ret = this->_vSafePrint(fmt, args);

    va_end(args);
    return ret;
}

int View::_moveAndPrint(const Point& contentPos, const char * const fmt, ...) const
{
    this->_moveCursor(contentPos);

    va_list args;

    va_start(args, fmt);

    const auto ret = this->_vPrint(fmt, args);

    va_end(args);
    return ret;
}

void View::_hasMoreTop(const bool hasMore)
{
    if (_moreTop == hasMore) {
        return;
    }

    _moreTop = hasMore;
    this->_decorate();
}

void View::_hasMoreBottom(const bool hasMore)
{
    if (_moreBottom == hasMore) {
        return;
    }

    _moreBottom = hasMore;
    this->_decorate();
}

int View::_vSafePrint(const char * const fmt, va_list& args) const
{
    int x, y;

    getyx(_curWindow, y, x);
    static_cast<void>(y);
    assert(x >= 0);

    if (static_cast<Index>(x) >= _contentRect.w) {
        return 0;
    }

    const auto availWidth = _contentRect.pos.x + _contentRect.w - x;

    assert(availWidth < _lineBuf.size() - 1);
    static_cast<void>(std::vsnprintf(_lineBuf.data(), availWidth + 1, fmt, args));
    return this->_print("%s", _lineBuf.data());
}

ViewInspectCmdStateObserverGuard::ViewInspectCmdStateObserverGuard(InspectCmdState& appState,
                                                                   View& view) :
    _observerGuard {appState, std::bind(&View::_appStateChanged, &view, std::placeholders::_1)}
{
}

} // namespace jacques
