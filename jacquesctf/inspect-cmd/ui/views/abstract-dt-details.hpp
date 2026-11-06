/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ABSTRACT_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ABSTRACT_DT_DETAILS_HPP

#include <memory>
#include <vector>
#include <string_view>
struct ncplane;
#include <yactfr/metadata/fwd.hpp>

#include "aliases.hpp"
#include "utils.hpp"
#include "../stylist.hpp"

namespace jacques {

/*
 * Number of terminal columns which AbstractDtDetails::_renderStr()
 * would use to render the string `str` (escaped; see
 * utils::escapeStr()) without width limit.
 */
Size dtDetailsStrWidth(std::string_view str);

class AbstractDtDetails
{
public:
    using UP = std::unique_ptr<const AbstractDtDetails>;

protected:
    explicit AbstractDtDetails(Size indent, const Stylist& stylist) noexcept;

public:
    virtual ~AbstractDtDetails() = default;
    void renderLine(ncplane& plane, Size maxWidth, bool stylize = false) const;

protected:
    Size _indent() const noexcept
    {
        return _indentWidth;
    }

    /*
     * Makes the next writes to `plane` use the style of `role`.
     */
    void _style(ncplane& plane, const Stylist::Role role) const
    {
        _theStylist->apply(plane, role);
    }

    virtual void _renderLine(ncplane& plane, Size maxWidth, bool stylize) const = 0;
    void _renderChar(ncplane& plane, Size& remWidth, char ch) const;
    void _renderStr(ncplane& plane, Size& remWidth, const char *str) const;

    void _renderStr(ncplane& plane, Size& remWidth, const std::string& str) const
    {
        this->_renderStr(plane, remWidth, str.c_str());
    }

private:
    const Size _indentWidth;
    const Stylist * const _theStylist;
};

template <typename IntRangeSetT>
std::string intRangeSetStr(const IntRangeSetT& ranges)
{
    std::vector<std::string> rangeStrs;

    std::transform(ranges.begin(), ranges.end(), std::back_inserter(rangeStrs),
                   [](auto& range) {
        std::ostringstream ss;

        ss << '[';

        if (range.lower() == range.upper()) {
            ss << range.lower();
        } else {
            ss << range.lower() << ", " << range.upper();
        }

        ss << ']';
        return ss.str();
    });

    return utils::csvListStr(rangeStrs, false, nullptr);
}

void dtDetailsFromDt(const yactfr::DataType& dt, const Stylist& stylist,
                     std::vector<AbstractDtDetails::UP>& vec);

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ABSTRACT_DT_DETAILS_HPP
