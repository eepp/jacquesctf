/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>
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

    // initially hidden (not attached): root of its own pile
    _curPlane = ncpile_create(&term::nc(), &opts);
    assert(_curPlane);

    /*
     * Make the cells which were never written (or erased) opaque
     * spaces instead of transparent glyphs.
     */
    ncplane_set_base(_curPlane, " ", 0, 0);
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

void View::detach()
{
    if (!_isAttached) {
        return;
    }

    // make it the root of a new pile, which we never render
    ncplane_reparent(_curPlane, _curPlane);
    _isAttached = false;
}

void View::render()
{
    if (!_isVisible) {
        this->detach();
        return;
    }

    if (_isInvalidated) {
        this->_style(Stylist::Role::STD);
        ncplane_erase(_curPlane);
        this->_redrawContent();

        // decorate last: _redrawContent() may change the "more" indicators
        this->_decorate();
        _isInvalidated = false;
    }

    if (!_isAttached) {
        ncplane_reparent(_curPlane, &term::stdPlane());
        ncplane_move_yx(_curPlane, static_cast<int>(_rect.pos.y), static_cast<int>(_rect.pos.x));
        _isAttached = true;
    }

    ncplane_move_top(_curPlane);
}

void View::moveAndResize(const Rect& rect)
{
    ncplane_move_yx(_curPlane, static_cast<int>(rect.pos.y), static_cast<int>(rect.pos.x));
    ncplane_resize_simple(_curPlane, rect.h, rect.w);
    _rect = rect;
    this->_setContentRect();
    this->_resized();
    this->_invalidate();
}

void View::_appStateChanged(Message)
{
}

bool View::_handleKey(Key)
{
    return false;
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
    this->_invalidate();
}

void View::blur()
{
    _hasFocusMemb = false;
    this->_invalidate();
}

void View::_decorate() const
{
    if (_decoStyle == DecorationStyle::BORDERLESS) {
        return;
    }

    // style of the border and title
    if (!_hasFocusMemb) {
        this->_style(Stylist::Role::FAINT);
    } else if (_decoStyle == DecorationStyle::BORDERS_EMPHASIZED) {
        this->_style(Stylist::Role::HIGHLIGHT, true);
    } else {
        this->_style(Stylist::Role::STD);
    }

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

    const auto title = std::invoke([this] {
        const auto maxWidth = _rect.w - 4;

        if (term::strWidth(_theTitle) > maxWidth) {
            return std::string {term::strFittingPrefix(_theTitle, maxWidth - 1)} + glyphs::ELLIPSIS;
        } else {
            return _theTitle;
        }
    });

    ncplane_putstr_yx(_curPlane, 0, 2, title.c_str());

    static auto moreStr = " +++ ";
    const auto moreX = static_cast<int>(_rect.w - 7);

    if (_moreTop) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        this->_style(Stylist::Role::HIGHLIGHT);
        ncplane_putstr_yx(_curPlane, 0, moreX, moreStr);
    }

    if (_moreBottom) {
        assert(_decoStyle != DecorationStyle::BORDERLESS);
        this->_style(Stylist::Role::HIGHLIGHT);
        ncplane_putstr_yx(_curPlane, static_cast<int>(_rect.h - 1), moreX, moreStr);
    }
}

void View::_clearContent(const Stylist::Role role) const
{
    this->_style(role);
    this->_fill({{0, 0}, _contentRect.w, _contentRect.h});
}

Index View::_write(const Point& contentPos, const std::string_view str) const
{
    if (contentPos.x >= _contentRect.w || contentPos.y >= _contentRect.h) {
        return contentPos.x;
    }

    const auto fittingStr = term::strFittingPrefix(str, _contentRect.w - contentPos.x);

    if (fittingStr.empty()) {
        return contentPos.x;
    }

    /*
     * Not using ncplane_putnstr_yx(): it could read past `fittingStr`
     * to complete the last grapheme cluster.
     */
    ncplane_putstr_yx(_curPlane, static_cast<int>(_contentRect.pos.y + contentPos.y),
                      static_cast<int>(_contentRect.pos.x + contentPos.x),
                      std::string {fittingStr}.c_str());
    return contentPos.x + term::strWidth(fittingStr);
}

void View::_fill(const Rect& contentRect) const
{
    if (contentRect.pos.x >= _contentRect.w || contentRect.pos.y >= _contentRect.h) {
        return;
    }

    const std::string spaces(std::min(contentRect.w, _contentRect.w - contentRect.pos.x), ' ');
    const auto endY = std::min(contentRect.pos.y + contentRect.h, _contentRect.h);

    for (auto y = contentRect.pos.y; y < endY; ++y) {
        this->_write({contentRect.pos.x, y}, spaces);
    }
}

void View::_clearRow(const Index contentY) const
{
    this->_fill({{0, contentY}, _contentRect.w, 1});
}

void View::_writeLabelValRows(const Point& contentPos, const std::vector<LabelValRow>& rows,
                              const Stylist::Role role, const bool emLabels) const
{
    Size labelWidth = 0;

    for (auto& row : rows) {
        labelWidth = std::max(labelWidth, term::strWidth(row.label));
    }

    auto y = contentPos.y;

    for (auto& row : rows) {
        this->_style(role, emLabels);
        this->_write({contentPos.x, y}, row.label);
        this->_style(role, !emLabels);
        this->_write({contentPos.x + labelWidth + 1, y}, row.val);
        ++y;
    }
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

void View::_moveCursor(const Point& contentPos) const
{
    const auto x = _contentRect.pos.x + contentPos.x;
    const auto y = _contentRect.pos.y + contentPos.y;

    static_cast<void>(ncplane_cursor_move_yx(_curPlane, static_cast<int>(y),
                                             static_cast<int>(x)));
}

ViewInspectCmdStateObserver::ViewInspectCmdStateObserver(InspectCmdState& appState,
                                                         View& view) :
    _curAppState {&appState},
    _observerGuard {appState, [&view](const Message msg) {
        view._appStateChanged(msg);
    }}
{
}

} // namespace jacques
