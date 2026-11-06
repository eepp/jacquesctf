/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_MAPPING_OR_FLAG_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_MAPPING_OR_FLAG_DETAILS_HPP

#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <string>
#include <sstream>
#include <algorithm>
#include <type_traits>
struct ncplane;
#include <yactfr/yactfr.hpp>

#include "abstract-dt-details.hpp"

namespace jacques {

class MappingOrFlagDetails final :
    public AbstractDtDetails
{
public:
    explicit MappingOrFlagDetails(std::string name, std::string rangesStr, Size indent,
                                  const Stylist& stylist);

private:
    void _renderLine(ncplane& plane, Size maxWidth, bool stylize) const override;

private:
    const std::string _name;
    const std::string _rangesStrs;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_MAPPING_OR_FLAG_DETAILS_HPP
