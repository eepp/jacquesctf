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
#include "stylist.hpp"
#include "utils.hpp"

namespace jacques {

View::View(const Rectangle& rect, const std::string& title,
           const DecorationStyle decoStyle, const Stylist& stylist) :
    _rect {rect},
    _myTitle {title},
    _decoStyle {decoStyle},
    _myStylist {&stylist},
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

    _contentRectangle = {
        Point {hasBorders ? 1ULL : 0, hasBorders ? 1ULL : 0},
        hasBorders ? _rect.w - 2 : _rect.w,
        hasBorders ? _rect.h - 2 : _rect.h
    };
}

void View::refresh(const bool touch) const
{
    if (!_visible) {
        return;
    }

    if (touch) {
        this->touch();
    }

    const auto ret = wnoutrefresh(_curWindow);

    assert(ret == OK);
    JACQUES_UNUSED(ret);
}

void View::redraw(const bool touch)
{
    _myStylist->std(*this);
    werase(_curWindow);
    this->_decorate();
    this->_redrawContent();
    this->refresh(touch);
}

void View::touch() const
{
    touchwin(_curWindow);
}

void View::moveAndResize(const Rectangle& rect)
{
    _myStylist->std(*this);
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

void View::_stateChanged(const Message& msg)
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
    _focused = true;
    this->_decorate();
}

void View::blur()
{
    _focused = false;
    this->_decorate();
}

void View::_decorate() const
{
    if (_decoStyle == DecorationStyle::BORDERLESS) {
        return;
    }

    _myStylist->viewBorder(*this, _focused,
                           _decoStyle == DecorationStyle::BORDERS_EMPHASIZED);
    box(_curWindow, 0, 0);

    std::string title;
    const auto maxWidth = _rect.w - 4;

    if (_myTitle.size() > maxWidth) {
        title = _myTitle.substr(0, maxWidth - 3);
        title += "...";
    } else {
        title = _myTitle;
    }

    _myStylist->viewTitle(*this, _focused,
                          _decoStyle == DecorationStyle::BORDERS_EMPHASIZED);

    const auto ret = mvwprintw(_curWindow, 0, 2, "%s", title.c_str());

    assert(ret == OK);
    JACQUES_UNUSED(ret);

    static auto moreStr = " +++ ";
    const auto moreX = _rect.w - 7;

    if (_moreTop) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _myStylist->viewHasMore(*this);
        mvwprintw(_curWindow, 0, moreX, moreStr);
    }

    if (_moreBottom) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _myStylist->viewHasMore(*this);
        mvwprintw(_curWindow, _rect.h - 1, moreX, moreStr);
    }
}

void View::_clearContent() const
{
    _myStylist->std(*this);

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

ViewStateObserverGuard::ViewStateObserverGuard(State& state, View& view) :
    _observerGuard {state, std::bind(&View::_stateChanged, &view,
                                     std::placeholders::_1)}
{
}

} // namespace jacques
