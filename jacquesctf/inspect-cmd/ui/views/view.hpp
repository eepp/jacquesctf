/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP

#include <string>
#include <memory>
#include <cassert>
#include <cstdarg>
#include <curses.h>
#include <boost/core/noncopyable.hpp>

#include "../rect.hpp"
#include "../../state/msg.hpp"
#include "../../state/state.hpp"

namespace jacques {

class Stylist;
class ViewStateObserverGuard;

/*
 * Base class of all views.
 *
 * A view manages an ncurses window. The `View` class provides many
 * useful protected methods to its derived classes, like
 * _clearContent(), _moveCursor(), and _safeMoveAndPrint().
 *
 * A view is built with a decoration style (see `DecorationStyle`):
 * either it has a border or none. When it has a border, it can have a
 * title, and can be focused or blurred (see focus() and blur()). Also,
 * when it has a title, the protected _hasMoreTop() and _hasMoreBottom()
 * methods control whether or not the top and bottom right "more
 * content" indicators exist.
 *
 * You can publicly refresh and redraw a view. See the document of the
 * refresh() and redraw() methods to learn more.
 *
 * The visibility (isVisible()) of a view controls whether or not this
 * view effectively refreshes its underlying ncurses window when you
 * call refresh().
 *
 * A derived view can subscribe to the state of the application with a
 * ViewStateObserverGuard instance. This RAII helper has an underlying
 * StateObserverGuard and uses _stateChanged() as the observer.
 *
 * The View class is friend with the Stylist class: the Stylist class
 * has a bunch of stylizing methods which apply to a view and it needs
 * to access its underlying ncurses window to change its current
 * attributes.
 */
class View :
    boost::noncopyable
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
     * `rect` is the rectangle of the view within the terminal screen.
     */
    explicit View(const Rect& rect, const std::string& title, DecorationStyle decoStyle,
                  const Stylist& stylist);

public:
    virtual ~View();

    /*
     * Refreshes this view, that is, copies the currently _changed_
     * characters of theview since the last refresh to the screen
     * buffer. If `touch` is true, touches this view before refreshing
     * (see touch()).
     *
     * This method only applies if this view is visible.
     *
     * This method doesn't trigger a redraw.
     */
    void refresh(bool touch = false) const;

    /*
     * Focuses this view (changes the decoration accordingly).
     *
     * Only applies when the decoration style of this view is
     * `DecorationStyle::BORDERS` or
     * `DecorationStyle::BORDERS_EMPHASIZED`.
     */
    void focus();

    /*
     * Blurs this view (changes the decoration accordingly).
     *
     * Only applies when the decoration style of this view is
     * `DecorationStyle::BORDERS` or
     * `DecorationStyle::BORDERS_EMPHASIZED`.
     */
    void blur();

    /*
     * Redraws this view. This method makes this view redraw every
     * character according to its current state, and then calls
     * `refresh(touch)`.
     */
    void redraw(bool touch = false);

    /*
     * Touches the window of this view: invalidates all the current
     * characters so that the next call to refresh() copies _all_ the
     * characters of this view to the screen buffer.
     */
    void touch() const;

    /*
     * Moves and resizes this view with the new rectangle `rect`.
     */
    void moveAndResize(const Rect& rect);

    /*
     * Sets the visibility of this view to `isVisible`.
     *
     * When this view isn't visible, refresh() has no effect.
     */
    void isVisible(const bool isVisible) noexcept
    {
        _isVisible = isVisible;
    }

    bool isVisible() const
    {
        return _isVisible;
    }

    /*
     * Rectangle of this view within the terminal screen.
     */
    const Rect& rect() const
    {
        return _rect;
    }

    /*
     * Content rectangle of this view relative to its rectangle.
     *
     * If this view has no borders, then the dimensions of the content
     * rectangle are the same as rect(), but the top-left position is
     * (0, 0).
     *
     * A lot of protected methods taking a `contentPos` parameter apply
     * to a position relative to the position of the content rectangle.
     */
    const Rect& contentRect() const
    {
        return _contentRect;
    }

protected:
    /*
     * Called by the state (through a `ViewStateObserverGuard`) when the
     * state changes.
     */
    virtual void _stateChanged(Message msg);

    /*
     * Implementation must redraw the whole content (everything in
     * contentRect()).
     */
    virtual void _redrawContent();

    /*
     * Called _after_ this view is resized (rect() and contentRect() are
     * already changed).
     */
    virtual void _resized();

    /*
     * Clears everything in the content rectangle (contentRect()) with
     * the Stylist::std() style.
     */
    void _clearContent() const;

    /*
     * Clears the whole view's rectangle (rect()) without applying any
     * specific style.
     */
    void _clearRect() const;

    /*
     * Draws the decoration of this view according to the current state
     * (decoration style, title, "more content" indicators).
     */
    void _decorate() const;

    /*
     * Sets the title of this view to `title`.
     *
     * This only applies when the decoration style of this view isn't
     * `DecorationStyle::BORDERLESS`.
     */
    void _title(const std::string& title) noexcept
    {
        _theTitle = utils::escapeStr(title);
    }

    /*
     * Underlying ncurses window of this view.
     */
    WINDOW *_window() const noexcept
    {
        return _curWindow;
    }

    /*
     * Stylist of this view.
     */
    const Stylist& _stylist() const noexcept
    {
        return *_theStylist;
    }

    /*
     * Puts a single character `ch` at `contentPos` relative to the
     * top-left position of contentRect().
     */
    void _putChar(const Point& contentPos, const chtype ch) const
    {
        mvwaddch(_curWindow, _contentRect.pos.y + contentPos.y,
                 _contentRect.pos.x + contentPos.x, ch);
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
     * This method isn't safe and could print outside the view. Use
     * _safePrint() for a safe version which truncates the formatted
     * string.
     */
    int _print(const char *fmt, ...) const;

    /*
     * Like _print(), but truncates the formatted string so that it's
     * not printed outside the view.
     */
    int _safePrint(const char *fmt, ...) const;

    /*
     * Moves the cursor at `contentPos` relative to the top-left
     * position of contentRect().
     */
    void _moveCursor(const Point& contentPos) const;

    /*
     * Calls _moveCursor(contentPos), and then _safePrint().
     */
    int _safeMoveAndPrint(const Point& contentPos, const char *fmt, ...) const;

    /*
     * Calls _moveCursor(contentPos), and then _print().
     */
    int _moveAndPrint(const Point& contentPos, const char *fmt, ...) const;

    bool _hasFocus() const
    {
        return _hasFocusMemb;
    }

    /*
     * Enables the top "more content" indicator of this view if
     * `hasMore` is true.
     *
     * This only applies when the decoration style of this view isn't
     * `DecorationStyle::BORDERLESS`.
     */
    void _hasMoreTop(bool hasMore);

    /*
     * Enables the "more content" indicator of this view if `hasMore` is
     * true.
     *
     * This only applies when the decoration style of this view isn't
     * `DecorationStyle::BORDERLESS`.
     */
    void _hasMoreBottom(bool hasMore);

    /*
     * Beeps/flashes the screen.
     */
    void _alert() const noexcept
    {
        beep();
    }

private:
    void _setContentRect();
    int _vSafePrint(const char * const fmt, va_list& args) const;

    int _vPrint(const char * const fmt, va_list& args) const
    {
        return vw_printw(_curWindow, fmt, args);
    }

private:
    Rect _rect;
    std::string _theTitle;
    const DecorationStyle _decoStyle;
    Rect _contentRect;
    const Stylist * const _theStylist;
    bool _hasFocusMemb = false;
    WINDOW* _curWindow;
    bool _isVisible = false;
    mutable std::vector<char> _lineBuf;
    bool _moreTop = false;
    bool _moreBottom = false;
};

/*
 * This is a state observer guard for views.
 *
 * Just have an instance and build it with the state to observe and the
 * observer view. When the state changes, the virtual
 * View::_stateChanged() method is called. When the view is destroyed,
 * the observer guard removes the view from the state observers.
 */
class ViewStateObserverGuard final
{
public:
    explicit ViewStateObserverGuard(State& state, View& view);

private:
    const StateObserverGuard _observerGuard;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP
