/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "enum-type-mapping-details.hpp"

namespace jacques {

EnumTypeMappingDetails::EnumTypeMappingDetails(std::string name, std::string rangesStr,
                                               const Size indent, const Stylist& stylist) :
    AbstractDtDetails {indent, stylist},
    _name {std::move(name)},
    _rangesStrs {std::move(rangesStr)}
{
}

void EnumTypeMappingDetails::_renderLine(WINDOW * const window, const Size maxWidth,
                                         const bool stylize) const
{
    auto remWidth = maxWidth;

    if (stylize) {
        this->_stylist().detailsViewEnumTypeMappingName(window);
    }

    this->_renderStr(window, remWidth, _name + ' ');

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_stylist().detailsViewIntRanges(window);
    }

    this->_renderStr(window, remWidth, _rangesStrs);
}

} // namespace jacques
