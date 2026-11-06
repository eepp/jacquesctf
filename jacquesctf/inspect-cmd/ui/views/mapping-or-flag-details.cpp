/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "mapping-or-flag-details.hpp"

namespace jacques {

MappingOrFlagDetails::MappingOrFlagDetails(std::string name, std::string rangesStr,
                                           const Size indent, const Stylist& stylist) :
    AbstractDtDetails {indent, stylist},
    _name {std::move(name)},
    _rangesStrs {std::move(rangesStr)}
{
}

void MappingOrFlagDetails::_renderLine(ncplane& plane, const Size maxWidth,
                                       const bool stylize) const
{
    auto remWidth = maxWidth;

    if (stylize) {
        this->_style(plane, Stylist::Role::HIGHLIGHT);
    }

    this->_renderStr(plane, remWidth, _name + ' ');

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_style(plane, Stylist::Role::ACCENT);
    }

    this->_renderStr(plane, remWidth, _rangesStrs);
}

} // namespace jacques
