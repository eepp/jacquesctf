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
    this->_style(Stylist::Role::STD);
    this->_drawCurText({1, 1}, _text);
}

void InputView::_drawBorder() const
{
    this->_style(Stylist::Role::INPUT_BORDER);

    // border made of spaces (borderless view: content rectangle is the rectangle)
    const auto& rect = this->contentRect();

    this->_clearRow(0);
    this->_clearRow(rect.h - 1);
    this->_fill({{0, 1}, 1, rect.h - 2});
    this->_fill({{rect.w - 1, 1}, 1, rect.h - 2});
}

} // namespace jacques
