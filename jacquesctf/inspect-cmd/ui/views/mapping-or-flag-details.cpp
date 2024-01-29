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

#include "mapping-or-flag-details.hpp"

namespace jacques {

MappingOrFlagDetails::MappingOrFlagDetails(std::string name, std::string rangesStr,
                                           const Size indent, const Stylist& stylist) :
    AbstractDtDetails {indent, stylist},
    _name {std::move(name)},
    _rangesStrs {std::move(rangesStr)}
{
}

void MappingOrFlagDetails::_renderLine(WINDOW * const window, const Size maxWidth,
                                       const bool stylize) const
{
    auto remWidth = maxWidth;

    if (stylize) {
        this->_stylist().detailsViewMappingOrFlagName(window);
    }

    this->_renderStr(window, remWidth, _name + ' ');

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_stylist().detailsViewMappingOrFlagRange(window);
    }

    this->_renderStr(window, remWidth, _rangesStrs);
}

} // namespace jacques
