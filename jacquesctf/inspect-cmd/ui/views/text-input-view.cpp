/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cctype>
#include <cstring>

#include "text-input-view.hpp"
#include "../stylist.hpp"

namespace jacques {

TextInputView::TextInputView(const Rect& rect, const Stylist& stylist) :
    InputView {rect, stylist}
{
}

void TextInputView::_drawCurText(const std::string& text)
{
    this->_style(Stylist::Role::STD);
    this->_print("%s", text.c_str());
}

} // namespace jacques
