/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_ENUM_DATA_TYPE_MEMBER_DETAILS_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_ENUM_DATA_TYPE_MEMBER_DETAILS_HPP

#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <string>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <curses.h>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/metadata/enum-type.hpp>

#include "abstract-data-type-details.hpp"

namespace jacques {

template <typename EnumTypeT>
class EnumDataTypeMemberDetails :
    public AbstractDataTypeDetails
{
public:
    using Member = typename EnumTypeT::Member;

public:
    explicit EnumDataTypeMemberDetails(const std::string& name,
                                       const Member& member, const Size indent,
                                       const Stylist& stylist) :
        AbstractDataTypeDetails {indent, stylist},
        _name {name},
        _member {&member}
    {
    }

private:
    void _renderLine(WINDOW *window, const Size maxWidth,
                     const bool stylize) const override
    {
        auto remWidth = maxWidth;

        if (stylize) {
            this->_stylist().detailsViewEnumDataTypeMemberName(window);
        }

        this->_renderString(window, remWidth, _name);

        if (remWidth == 0) {
            return;
        }

        std::ostringstream ss;

        for (auto& range : _member->ranges()) {
            ss.str(std::string {});
            ss.clear();

            if (stylize) {
                this->_stylist().std(window);
            }

            this->_renderChar(window, remWidth, ' ');

            if (remWidth == 0) {
                return;
            }

            if (range.lower() == range.upper()) {
                ss << '[' << range.lower() << ']';
            } else {
                ss << '[' << range.lower() << ", " << range.upper() << ']';
            }

            if (stylize) {
                this->_stylist().detailsViewEnumDataTypeMemberRange(window);
            }

            this->_renderString(window, remWidth, ss.str());

            if (remWidth == 0) {
                return;
            }
        }
    }

private:
    const std::string _name;
    const Member *_member;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_ENUM_DATA_TYPE_MEMBER_DETAILS_HPP
