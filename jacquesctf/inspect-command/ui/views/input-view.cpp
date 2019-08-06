/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cctype>
#include <cstring>

#include "input-view.hpp"
#include "stylist.hpp"
#include "utils.hpp"

namespace jacques {

InputView::InputView(const Rectangle& rect, const Stylist& stylist) :
    View {rect, "Input", DecorationStyle::BORDERLESS, stylist}
{
    assert(rect.h == 3);
}

InputView::~InputView()
{
}

void InputView::_redrawContent()
{
    this->_clearContent();
    this->_drawBorder();
}

void InputView::_drawBorder() const
{
    this->_stylist().simpleInputViewBorder(*this);
    wborder(this->_window(), ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
}

void InputView::drawCurrentText(const std::string& text)
{
    // clear input first
    this->_stylist().std(*this);

    for (Index x = 1; x < this->contentRect().w - 2; ++x) {
        this->_putChar({x, 1}, ' ');
    }

    this->_moveCursor({1, 1});
    this->_drawCurrentText(text);
}

} // namespace jacques
