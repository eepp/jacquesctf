/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ABSTRACT_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ABSTRACT_DT_DETAILS_HPP

#include <memory>
#include <cstring>
#include <vector>
#include <curses.h>
#include <boost/optional.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "aliases.hpp"
#include "utils.hpp"
#include "../stylist.hpp"

namespace jacques {

class AbstractDtDetails
{
public:
    using UP = std::unique_ptr<const AbstractDtDetails>;

protected:
    explicit AbstractDtDetails(Size indent, const Stylist& stylist) noexcept;

public:
    virtual ~AbstractDtDetails() = default;
    void renderLine(WINDOW *window, Size maxWidth, bool stylize = false) const;

protected:
    Size _indent() const noexcept
    {
        return _indentWidth;
    }

    const Stylist& _stylist() const noexcept
    {
        return *_theStylist;
    }

    virtual void _renderLine(WINDOW *window, Size maxWidth, bool stylize) const = 0;
    void _renderChar(WINDOW *window, Size& remWidth, char ch) const;
    void _renderStr(WINDOW *window, Size& remWidth, const char *str) const;

    void _renderStr(WINDOW *window, Size& remWidth, const std::string& str) const
    {
        this->_renderStr(window, remWidth, str.c_str());
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
