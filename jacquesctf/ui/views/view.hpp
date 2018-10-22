/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_VIEW_HPP
#define _JACQUES_VIEW_HPP

#include <string>
#include <memory>
#include <cassert>
#include <cstdarg>
#include <curses.h>

#include "rectangle.hpp"
#include "message.hpp"
#include "state.hpp"

namespace jacques {

class Stylist;
class ViewStateObserverGuard;

/*
 * Base class of all views.
 *
 * A view manages an ncurses window. The View class provides many useful
 * protected methods to its derived classes, like View::_clearContent(),
 * View::_moveCursor(), and View::_safeMoveAndPrint().
 *
 * A view is built with a decoration style (see DecorationStyle): either
 * it has a border or none. When it has a border, it can have a title,
 * and can be focused or blurred (see View::focus() and View::blur()).
 * Also, when it has a title, the protected View::_hasMoreTop() and
 * View::_hasMoreBottom() control whether or not the top and bottom
 * right "more content" indicator exists.
 *
 * You can publicly refresh and redraw a view. See the View::refresh()
 * and View::redraw() methods's documentation for more information.
 *
 * The visibility (View::isVisible()) of a view controls whether or not
 * this view effectively refreshes its underlying ncurses window when
 * you call View::refresh().
 *
 * A derived view can subscribe to the application's state with a
 * ViewStateObserverGuard instance. This RAII helper has an underlying
 * StateObserverGuard and uses View::_stateChanged() as the observer.
 *
 * The View class is friend with the Stylist class: the Stylist class
 * has a bunch of stylizing methods which apply to a view and it needs
 * to access its underlying ncurses window to change its current
 * attributes.
 */
class View
{
    friend class Stylist;
    friend class ViewStateObserverGuard;

public:
    enum class DecorationStyle {
        // no borders; plain view
        BORDERLESS,

        // normal borders
        BORDERS,

        // emphasized borders (different border style)
        BORDERS_EMPHASIZED,
    };

protected:
    /*
     * Builds a base view.
     *
     * `rect` is the view's rectangle within the terminal screen.
     */
    explicit View(const Rectangle& rect, const std::string& title,
                  DecorationStyle decoStyle, const Stylist& stylist);

public:
    virtual ~View();

    /*
     * Refreshes this view, that is, copies the view's currently
     * _changed_ characters since the last refresh to the screen buffer.
     * If `touch` is true, touches this view before refreshing (see
     * View::touch()).
     *
     * This method only applies if the view is visible.
     *
     * This method does not trigger a redraw.
     */
    void refresh(bool touch = false) const;

    /*
     * Focuses this view (changes the decoration accordingly).
     *
     * Only applies when the view's decoration style is
     * DecorationStyle::BORDERS or DecorationStyle::BORDERS_EMPHASIZED.
     */
    void focus();

    /*
     * Blurs this view (changes the decoration accordingly).
     *
     * Only applies when the view's decoration style is
     * DecorationStyle::BORDERS or DecorationStyle::BORDERS_EMPHASIZED.
     */
    void blur();

    /*
     * Redraws this view. This method makes the view redraw every
     * character according to its current state, and then calls
     * View::refresh(touch).
     */
    void redraw(bool touch = false);

    /*
     * Touches this view's window: invalidates all the current
     * characters so that the next call to View::refresh() copies _all_
     * the view's characters to the screen buffer.
     */
    void touch() const;

    /*
     * Moves and resizes this view with the new rectangle `rect`.
     */
    void moveAndResize(const Rectangle& rect);

    /*
     * Sets this view's visibility to `isVisible`.
     *
     * When the view is not visible, View::refresh() has no effect.
     */
    void isVisible(const bool isVisible)
    {
        _visible = isVisible;
    }

    bool isVisible() const
    {
        return _visible;
    }

    /*
     * This view's rectangle within the terminal screen.
     */
    const Rectangle& rect() const
    {
        return _rect;
    }

    /*
     * This view's content rectangle relative to the view's rectangle.
     *
     * If this view has no borders, then the content rectangle's
     * dimensions are the same as View::rect(), but the position is
     * {0, 0}.
     *
     * A lot of protected methods taking a `contentPos` parameter apply
     * to a position relative to the content rectangle's position.
     */
    const Rectangle& contentRect() const
    {
        return _contentRectangle;
    }

protected:
    /*
     * Called by the state (through a ViewStateObserverGuard) when the
     * state changes.
     */
    virtual void _stateChanged(const Message& msg);

    /*
     * Implementation must redraw the whole content (everything in
     * View::contentRect()).
     */
    virtual void _redrawContent();

    /*
     * Called _after_ the view is resized (View::rect() and
     * View::contentRect() are already changed).
     */
    virtual void _resized();

    /*
     * Clears everything in the content rectangle (View::contentRect())
     * with the Stylist::std() style.
     */
    void _clearContent() const;

    /*
     * Clears the whole view's rectangle (View::rect()) without applying
     * any specific style.
     */
    void _clearRect() const;

    /*
     * Draws the view's decoration according to the current state
     * (decoration style, title, "more content" indicators).
     */
    void _decorate() const;

    /*
     * Sets the view's title to `title`.
     *
     * This only applies when the view's decoration style is not
     * DecorationStyle::BORDERLESS.
     */
    void _title(const std::string& title) noexcept
    {
        _myTitle = title;
    }

    /*
     * View's underlying ncurses window.
     */
    WINDOW *_window() const noexcept
    {
        return _curWindow;
    }

    /*
     * View's stylist.
     */
    const Stylist& _stylist() const noexcept
    {
        return *_myStylist;
    }

    /*
     * Puts a single character `ch` at `contentPos` relative to
     * View::contentRect().
     */
    void _putChar(const Point& contentPos, const chtype ch) const
    {
        mvwaddch(_curWindow, _contentRectangle.pos.y + contentPos.y,
                 _contentRectangle.pos.x + contentPos.x, ch);
    }

    /*
     * Appends a single character `ch` at the current cursor position.
     */
    void _appendChar(const chtype ch) const
    {
        waddch(_curWindow, ch);
    }

    /*
     * Prints a formatted string at the current cursor position.
     *
     * This method is not safe and could print outside the view. Use
     * View::_safePrint() for a safe version which truncates the
     * formatted string.
     */
    int _print(const char *fmt, ...) const
    {
        va_list args;

        va_start(args, fmt);

        const auto ret = this->_vPrint(fmt, args);

        va_end(args);
        return ret;
    }

    /*
     * Like View::_print(), but truncates the formatted string so that
     * it's not printed outside the view.
     */
    int _safePrint(const char *fmt, ...) const
    {
        va_list args;

        va_start(args, fmt);

        const auto ret = this->_vSafePrint(fmt, args);

        va_end(args);
        return ret;
    }

    /*
     * Moves the cursor at `contentPos` relative to View::contentRect().
     */
    void _moveCursor(const Point& contentPos) const
    {
        const auto x = _contentRectangle.pos.x + contentPos.x;
        const auto y = _contentRectangle.pos.y + contentPos.y;

        (void) wmove(_curWindow, y, x);
    }

    /*
     * Calls View::_moveCursor(contentPos), and then View::_safePrint().
     */
    int _safeMoveAndPrint(const Point& contentPos, const char *fmt, ...) const
    {
        this->_moveCursor(contentPos);

        va_list args;

        va_start(args, fmt);

        const auto ret = this->_vSafePrint(fmt, args);

        va_end(args);
        return ret;
    }

    /*
     * Calls View::_moveCursor(contentPos), and then View::_print().
     */
    int _moveAndPrint(const Point& contentPos, const char *fmt, ...) const
    {
        this->_moveCursor(contentPos);

        va_list args;

        va_start(args, fmt);

        const auto ret = this->_vPrint(fmt, args);

        va_end(args);
        return ret;
    }

    bool _hasFocus() const
    {
        return _focused;
    }

    /*
     * Enables this view's top "more content" indicator if `hasMore` is
     * true.
     *
     * This only applies when the view's decoration style is not
     * DecorationStyle::BORDERLESS.
     */
    void _hasMoreTop(const bool hasMore)
    {
        if (_moreTop == hasMore) {
            return;
        }

        _moreTop = hasMore;
        this->_decorate();
    }

    /*
     * Enables this view's bottom "more content" indicator if `hasMore`
     * is true.
     *
     * This only applies when the view's decoration style is not
     * DecorationStyle::BORDERLESS.
     */
    void _hasMoreBottom(const bool hasMore)
    {
        if (_moreBottom == hasMore) {
            return;
        }

        _moreBottom = hasMore;
        this->_decorate();
    }

private:
    void _setContentRect();

    int _vSafePrint(const char *fmt, va_list& args) const
    {
        int x, y;

        getyx(_curWindow, y, x);
        JACQUES_UNUSED(y);
        assert(x >= 0);

        if (static_cast<Index>(x) >= _contentRectangle.w) {
            return 0;
        }

        const auto availWidth = _contentRectangle.pos.x + _contentRectangle.w - x;

        assert(availWidth < _lineBuf.size() - 1);
        (void) std::vsnprintf(_lineBuf.data(), availWidth + 1, fmt, args);
        return this->_print("%s", _lineBuf.data());
    }

    int _vPrint(const char *fmt, va_list& args) const
    {
        return vwprintw(_curWindow, fmt, args);
    }

private:
    Rectangle _rect;
    std::string _myTitle;
    const DecorationStyle _decoStyle;
    Rectangle _contentRectangle;
    const Stylist * const _myStylist;
    bool _focused = false;
    WINDOW* _curWindow;
    bool _visible = false;
    mutable std::vector<char> _lineBuf;
    bool _moreTop = false;
    bool _moreBottom = false;
};

/*
 * This is a state observer guard for views. Just have an instance and
 * build it with the state to observe and the observer view. When the
 * state changes, the virtual View::_stateChanged() method is called.
 * When the view is destroyed, the observer guard removes the view from
 * the state's observers.
 */
class ViewStateObserverGuard
{
public:
    explicit ViewStateObserverGuard(State& state, View& view);

private:
    const StateObserverGuard _observerGuard;
};

} // namespace jacques

#endif // _JACQUES_VIEW_HPP
