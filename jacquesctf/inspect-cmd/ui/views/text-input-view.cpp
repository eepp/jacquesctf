/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
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
    this->_stylist().std(*this);
    this->_print("%s", text.c_str());
}

} // namespace jacques
