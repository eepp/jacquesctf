/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <array>
#include <cstring>
#include <yactfr/yactfr.hpp>

#include "abstract-dt-details.hpp"
#include "dt-details.hpp"
#include "enum-type-mapping-details.hpp"

namespace jacques {

AbstractDtDetails::AbstractDtDetails(const Size indent,
                                                 const Stylist& stylist) noexcept :
    _indentWidth {indent},
    _theStylist {&stylist}
{
}

void AbstractDtDetails::renderLine(WINDOW * const window, const Size maxWidth, const bool stylize) const
{
    if (_indentWidth >= maxWidth) {
        return;
    }

    // indent
    for (Index i = 0; i < _indentWidth; ++i) {
        waddch(window, ' ');
    }

    this->_renderLine(window, maxWidth - _indentWidth, stylize);
}

void AbstractDtDetails::_renderChar(WINDOW * const window, Size& remWidth, const char ch) const
{
    if (remWidth > 0) {
        waddch(window, ch);
        --remWidth;
    }
}

void AbstractDtDetails::_renderStr(WINDOW * const window, Size& remWidth, const char * const str) const
{
    assert(remWidth != 0);

    std::array<char, 128> buf;
    const auto maxPrintSize = std::min(remWidth + 1, static_cast<Size>(buf.size()));

    std::snprintf(buf.data(), maxPrintSize, "%s", utils::escapeStr(str).c_str());
    remWidth -= std::strlen(buf.data());
    wprintw(window, "%s", buf.data());
}

} // namespace jacques
