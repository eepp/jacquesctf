/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ENUM_TYPE_MAPPING_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ENUM_TYPE_MAPPING_DETAILS_HPP

#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <string>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <curses.h>
#include <yactfr/yactfr.hpp>

#include "abstract-dt-details.hpp"

namespace jacques {

class EnumTypeMappingDetails final :
    public AbstractDtDetails
{
public:
    explicit EnumTypeMappingDetails(std::string name, std::string rangesStr, Size indent,
                                    const Stylist& stylist);

private:
    void _renderLine(WINDOW *window, Size maxWidth, bool stylize) const override;

private:
    const std::string _name;
    const std::string _rangesStrs;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ENUM_TYPE_MAPPING_DETAILS_HPP
