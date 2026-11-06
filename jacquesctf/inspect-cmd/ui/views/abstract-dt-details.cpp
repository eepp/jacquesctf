/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <string>
#include <string_view>
#include <yactfr/yactfr.hpp>
#include <notcurses/notcurses.h>

#include "abstract-dt-details.hpp"
#include "dt-details.hpp"
#include "mapping-or-flag-details.hpp"
#include "../term.hpp"
#include "utils.hpp"

namespace jacques {

AbstractDtDetails::AbstractDtDetails(const Size indent,
                                                 const Stylist& stylist) noexcept :
    _indentWidth {indent},
    _theStylist {&stylist}
{
}

void AbstractDtDetails::renderLine(ncplane& plane, const Size maxWidth, const bool stylize) const
{
    if (_indentWidth >= maxWidth) {
        return;
    }

    // indent
    for (Index i = 0; i < _indentWidth; ++i) {
        ncplane_putchar(&plane, ' ');
    }

    this->_renderLine(plane, maxWidth - _indentWidth, stylize);
}

void AbstractDtDetails::_renderChar(ncplane& plane, Size& remWidth, const char ch) const
{
    if (remWidth > 0) {
        ncplane_putchar(&plane, ch);
        --remWidth;
    }
}

void AbstractDtDetails::_renderStr(ncplane& plane, Size& remWidth, const char * const str) const
{
    assert(remWidth != 0);

    const auto escapedStr = utils::escapeStr(str);
    const std::string fittingStr {term::strFittingPrefix(escapedStr, remWidth)};

    remWidth -= term::strWidth(fittingStr);
    ncplane_putstr(&plane, fittingStr.c_str());
}

Size dtDetailsStrWidth(const std::string_view str)
{
    return term::strWidth(utils::escapeStr(str));
}

} // namespace jacques
