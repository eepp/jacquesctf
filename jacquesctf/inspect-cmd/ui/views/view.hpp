/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <cassert>
#include <cstdarg>
#include <boost/filesystem/path.hpp>

#include "../rect.hpp"
#include "../../state/msg.hpp"
#include "../../state/inspect-cmd-state.hpp"

struct ncplane;

namespace jacques {

class Stylist;
class ViewInspectCmdStateObserverGuard;

/*
 * Some non-ASCII glyphs which views use.
 */
namespace glyphs {

constexpr auto VLINE = "│";
constexpr auto BULLET = "·";
constexpr auto CKBOARD = "▒";
constexpr auto RARROW = "→";
constexpr auto ELLIPSIS = "…";

} // namespace glyphs

/*
 * Base class of all views.
 *
 * A view manages a notcurses plane. The `View` class provides many
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
 * view effectively shows its underlying plane when you call refresh().
 * An invisible view never shows its plane.
 *
 * A derived view can subscribe to the state of the application with a
 * ViewInspectCmdStateObserverGuard instance. This RAII helper has an
 * underlying StateObserverGuard and uses _appStateChanged() as the
 * observer.
 *
 * The View class is friend with the Stylist class: the Stylist class
 * has a bunch of stylizing methods which apply to a view and it needs
 * to access its underlying plane to change its current attributes.
 */
class View
{
    friend class Stylist;
    friend class ViewInspectCmdStateObserverGuard;

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
    explicit View(const Rect& rect, std::string_view title, DecorationStyle decoStyle,
                  const Stylist& stylist);

public:
    virtual ~View();

    View(const View&) = delete;
    View& operator=(const View&) = delete;

    /*
     * Refreshes this view, that is, shows its plane on top of the
     * other planes on the next terminal update (term::update()).
     *
     * This method only applies if this view is visible.
     *
     * This method doesn't trigger a redraw.
     */
    void refresh() const;

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
     * refresh().
     */
    void redraw();

    /*
     * Moves and resizes this view with the new rectangle `rect`.
     */
    void moveAndResize(const Rect& rect);

    /*
     * Sets the visibility of this view to `isVisible`.
     *
     * When this view isn't visible, its plane is hidden and refresh()
     * has no effect.
     */
    void isVisible(bool isVisible);

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
     * Called by the application state (through a
     * `ViewInspectCmdStateObserverGuard`) when the state changes.
     */
    virtual void _appStateChanged(Message msg);

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
    void _title(const std::string_view title) noexcept
    {
        _theTitle = utils::escapeStr(title);
    }

    /*
     * Underlying plane of this view.
     */
    ncplane *_plane() const noexcept
    {
        return _curPlane;
    }

    /*
     * Stylist of this view.
     */
    const Stylist& _stylist() const noexcept
    {
        return *_theStylist;
    }

    /*
     * Puts a single ASCII character `ch` at `contentPos` relative to
     * the top-left position of contentRect().
     */
    void _putChar(const Point& contentPos, char ch) const;

    /*
     * Puts a single glyph `glyph` (UTF-8; see `glyphs`) at
     * `contentPos` relative to the top-left position of contentRect().
     */
    void _putChar(const Point& contentPos, const char *glyph) const;

    /*
     * Appends a single ASCII character `ch` at the current cursor
     * position.
     */
    void _appendChar(char ch) const;

    /*
     * Appends a single character `ch` (Unicode code point) at the
     * current cursor position.
     */
    void _appendCh(char32_t ch) const;

    /*
     * Puts the UTF-8 string `str` at `contentPos` relative to the
     * top-left position of contentRect().
     *
     * This method doesn't truncate `str` (see
     * term::strFittingPrefix()).
     */
    void _putStr(const Point& contentPos, std::string_view str) const;

    /*
     * Appends the UTF-8 string `str` at the current cursor position.
     *
     * This method doesn't truncate `str` (see
     * term::strFittingPrefix()).
     */
    void _appendStr(std::string_view str) const;

    /*
     * Formats the path `path` (escaped UTF-8; see utils::escapeStr())
     * so that it occupies at most `maxWidth` terminal columns.
     *
     * The first element of the returned pair is the directory name and
     * the second element is the file name. Each one starts with
     * `glyphs::ELLIPSIS` if, due to `maxWidth`, it's incomplete.
     *
     * The widths of both returned elements plus one (to add a path
     * separator between them) is always less than or equal to
     * `maxWidth`.
     */
    static std::pair<std::string, std::string> _formatPath(const boost::filesystem::path& path,
                                                           Size maxWidth);

    /*
     * Prints a formatted string at the current cursor position.
     *
     * This method doesn't print outside the view, but there's no
     * explicit truncation. Use _safePrint() for a safe version which
     * truncates the formatted string so that it doesn't exceed the
     * content rectangle.
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

private:
    void _setContentRect();
    int _vSafePrint(const char * const fmt, va_list& args) const;
    int _vPrint(const char * const fmt, va_list& args) const;
    void _show() const;
    void _hide() const;

private:
    Rect _rect;
    std::string _theTitle;
    const DecorationStyle _decoStyle;
    Rect _contentRect;
    const Stylist * const _theStylist;
    bool _hasFocusMemb = false;
    ncplane *_curPlane;

    /*
     * True if `_curPlane` is bound to the standard plane (shown);
     * otherwise, it's the root of its own pile (hidden).
     */
    mutable bool _isShown = false;

    bool _isVisible = false;
    bool _moreTop = false;
    bool _moreBottom = false;
};

/*
 * This is an application state observer guard for views.
 *
 * Just have an instance and build it with the state to observe and the
 * observer view. When the state changes, the virtual
 * View::_appStateChanged() method is called. When the view is
 * destroyed, the observer guard removes the view from the state
 * observers.
 */
class ViewInspectCmdStateObserverGuard final
{
public:
    explicit ViewInspectCmdStateObserverGuard(InspectCmdState& state, View& view);

private:
    const InspectCmdStateObserverGuard _observerGuard;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_VIEW_HPP
