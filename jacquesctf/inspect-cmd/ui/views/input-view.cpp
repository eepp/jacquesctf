/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cctype>
#include <cstring>

#include "input-view.hpp"
#include "../stylist.hpp"
#include "utils.hpp"

namespace jacques {

InputView::InputView(const Rect& rect, const Stylist& stylist) :
    View {rect, "Input", DecorationStyle::BORDERLESS, stylist}
{
    assert(rect.h == 3);
}

void InputView::_redrawContent()
{
    this->_clearContent();
    this->_drawBorder();
    this->_stylist().std(*this);
    this->_moveCursor({1, 1});
    this->_drawCurText(_text);
}

void InputView::_drawBorder() const
{
    this->_stylist().simpleInputViewBorder(*this);

    // border made of spaces (borderless view: content rectangle is the rectangle)
    const auto& rect = this->contentRect();

    for (Index x = 0; x < rect.w; ++x) {
        this->_putChar({x, 0}, ' ');
        this->_putChar({x, rect.h - 1}, ' ');
    }

    for (Index y = 1; y < rect.h - 1; ++y) {
        this->_putChar({0, y}, ' ');
        this->_putChar({rect.w - 1, y}, ' ');
    }
}

} // namespace jacques
