/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <string>
#include <mutex>
#include <notcurses/notcurses.h>

#include "view.hpp"
#include "../stylist.hpp"
#include "../term.hpp"
#include "utils.hpp"

namespace jacques {

View::View(const Rect& rect, const std::string_view title, const DecorationStyle decoStyle,
           const Stylist& stylist) :
    _rect {rect},
    _theTitle {utils::escapeStr(title)},
    _decoStyle {decoStyle},
    _theStylist {&stylist}
{
    this->_setContentRect();

    ncplane_options opts {};

    opts.y = static_cast<int>(rect.pos.y);
    opts.x = static_cast<int>(rect.pos.x);
    opts.rows = rect.h;
    opts.cols = rect.w;

    // initially hidden: root of its own pile
    _curPlane = ncpile_create(&term::nc(), &opts);
    assert(_curPlane);

    /*
     * Make the cells which were never written (or erased) opaque
     * spaces instead of transparent glyphs.
     */
    ncplane_set_base(_curPlane, " ", 0, 0);
    this->redraw();
}

View::~View()
{
    ncplane_destroy(_curPlane);
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

void View::_show() const
{
    if (!_isShown) {
        ncplane_reparent(_curPlane, &term::stdPlane());
        ncplane_move_yx(_curPlane, static_cast<int>(_rect.pos.y), static_cast<int>(_rect.pos.x));
        _isShown = true;
    }

    ncplane_move_top(_curPlane);
}

void View::_hide() const
{
    if (!_isShown) {
        return;
    }

    // make it the root of a new pile, which we never render
    ncplane_reparent(_curPlane, _curPlane);
    _isShown = false;
}

void View::isVisible(const bool isVisible)
{
    _isVisible = isVisible;

    if (!isVisible) {
        this->_hide();
    }
}

void View::refresh() const
{
    if (!_isVisible) {
        return;
    }

    this->_show();
}

void View::redraw()
{
    _theStylist->std(*this);
    ncplane_erase(_curPlane);
    this->_decorate();
    this->_redrawContent();
    this->refresh();
}

void View::moveAndResize(const Rect& rect)
{
    ncplane_move_yx(_curPlane, static_cast<int>(rect.pos.y), static_cast<int>(rect.pos.x));
    ncplane_resize_simple(_curPlane, rect.h, rect.w);
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

    {
        // draw the border cell by cell with the current plane style
        const auto right = static_cast<int>(_rect.w - 1);
        const auto bottom = static_cast<int>(_rect.h - 1);

        for (auto x = 1; x < right; ++x) {
            ncplane_putegc_yx(_curPlane, 0, x, "─", nullptr);
            ncplane_putegc_yx(_curPlane, bottom, x, "─", nullptr);
        }

        for (auto y = 1; y < bottom; ++y) {
            ncplane_putegc_yx(_curPlane, y, 0, glyphs::VLINE, nullptr);
            ncplane_putegc_yx(_curPlane, y, right, glyphs::VLINE, nullptr);
        }

        ncplane_putegc_yx(_curPlane, 0, 0, "┌", nullptr);
        ncplane_putegc_yx(_curPlane, 0, right, "┐", nullptr);
        ncplane_putegc_yx(_curPlane, bottom, 0, "└", nullptr);
        ncplane_putegc_yx(_curPlane, bottom, right, "┘", nullptr);
    }

    const auto maxWidth = _rect.w - 4;
    const auto title = std::invoke([this, maxWidth] {
        if (term::strWidth(_theTitle) > maxWidth) {
            return std::string {term::strFittingPrefix(_theTitle, maxWidth - 1)} + glyphs::ELLIPSIS;
        } else {
            return _theTitle;
        }
    });

    _theStylist->viewTitle(*this, _hasFocusMemb, _decoStyle == DecorationStyle::BORDERS_EMPHASIZED);
    ncplane_putstr_yx(_curPlane, 0, 2, title.c_str());

    static auto moreStr = " +++ ";
    const auto moreX = static_cast<int>(_rect.w - 7);

    if (_moreTop) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _theStylist->viewHasMore(*this);
        ncplane_putstr_yx(_curPlane, 0, moreX, moreStr);
    }

    if (_moreBottom) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        _theStylist->viewHasMore(*this);
        ncplane_putstr_yx(_curPlane, static_cast<int>(_rect.h - 1), moreX, moreStr);
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
        ncplane_putchar_yx(_curPlane, static_cast<int>(y), 0, ' ');

        for (Index x = 1; x < _rect.w; ++x) {
            this->_appendChar(' ');
        }
    }
}

void View::_putChar(const Point& contentPos, const char ch) const
{
    ncplane_putchar_yx(_curPlane, static_cast<int>(_contentRect.pos.y + contentPos.y),
                       static_cast<int>(_contentRect.pos.x + contentPos.x), ch);
}

void View::_putChar(const Point& contentPos, const char * const glyph) const
{
    ncplane_putegc_yx(_curPlane, static_cast<int>(_contentRect.pos.y + contentPos.y),
                      static_cast<int>(_contentRect.pos.x + contentPos.x), glyph, nullptr);
}

void View::_appendChar(const char ch) const
{
    ncplane_putchar(_curPlane, ch);
}

void View::_appendCh(const char32_t ch) const
{
    ncplane_pututf32_yx(_curPlane, -1, -1, ch);
}

void View::_putStr(const Point& contentPos, const std::string_view str) const
{
    this->_moveCursor(contentPos);
    this->_appendStr(str);
}

void View::_appendStr(const std::string_view str) const
{
    /*
     * Not using ncplane_putnstr(): it could read past `str` to complete
     * the last grapheme cluster.
     */
    ncplane_putstr(_curPlane, std::string {str}.c_str());
}

std::pair<std::string, std::string> View::_formatPath(const boost::filesystem::path& path,
                                                      const Size maxWidth)
{
    const auto filename = path.filename().string();
    const auto dirName = path.parent_path().string();
    std::string filenameStr;

    if (term::strWidth(filename) > maxWidth) {
        filenameStr = glyphs::ELLIPSIS;
        filenameStr += term::strFittingSuffix(filename, maxWidth >= 1 ? maxWidth - 1 : 0);
    } else {
        filenameStr = filename;
    }

    const auto filenameWidth = term::strWidth(filenameStr);
    const auto leftWidth = maxWidth > filenameWidth ? maxWidth - filenameWidth : 0;
    std::string dirNameStr;

    if (leftWidth >= 2) {
        // enough space for the ellipsis and implicit `/`
        if (term::strWidth(dirName) < leftWidth) {
            dirNameStr = dirName;
        } else {
            dirNameStr = glyphs::ELLIPSIS;
            dirNameStr += term::strFittingSuffix(dirName, leftWidth - 2);
        }
    }

    return std::pair {std::move(dirNameStr), std::move(filenameStr)};
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

    static_cast<void>(ncplane_cursor_move_yx(_curPlane, static_cast<int>(y),
                                             static_cast<int>(x)));
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

int View::_vPrint(const char * const fmt, va_list& args) const
{
    return ncplane_vprintf(_curPlane, fmt, args);
}

int View::_vSafePrint(const char * const fmt, va_list& args) const
{
    unsigned int x;

    ncplane_cursor_yx(_curPlane, nullptr, &x);

    const auto endX = _contentRect.pos.x + _contentRect.w;

    if (static_cast<Index>(x) >= endX) {
        return 0;
    }

    // format the whole string first
    va_list argsCopy;

    va_copy(argsCopy, args);

    const auto len = std::vsnprintf(nullptr, 0, fmt, argsCopy);

    va_end(argsCopy);

    if (len <= 0) {
        return 0;
    }

    std::string str(static_cast<std::size_t>(len), '\0');

    static_cast<void>(std::vsnprintf(str.data(), str.size() + 1, fmt, args));

    // then only print what fits in the rest of the content rectangle
    return ncplane_putstr(_curPlane, std::string {term::strFittingPrefix(str, endX - x)}.c_str());
}

ViewInspectCmdStateObserverGuard::ViewInspectCmdStateObserverGuard(InspectCmdState& appState,
                                                                   View& view) :
    _observerGuard {appState, [&view](const Message msg) {
        /*
         * The state can change in a thread other than the main one (see
         * `InspectScreen::_search()`): lock the terminal while the view
         * modifies its plane.
         */
        const std::lock_guard<std::recursive_mutex> lock {term::mutex()};

        view._appStateChanged(msg);
    }}
{
}

} // namespace jacques
