/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TIME_OPS_HPP
#define _JACQUES_DATA_TIME_OPS_HPP

#include <cassert>
#include <cstdint>
#include <ostream>
#include <boost/operators.hpp>
#include <yactfr/element.hpp>
#include <yactfr/metadata/clock-type.hpp>

#include "ts.hpp"
#include "duration.hpp"

namespace jacques {
namespace internal {

static inline unsigned long long diff(const Ts& left, const Ts& right) noexcept
{
    assert(left >= right);

    if (left.nsFromOrigin() >= 0 && right.nsFromOrigin() < 0) {
        return static_cast<unsigned long long>(left.nsFromOrigin()) +
               static_cast<unsigned long long>(-right.nsFromOrigin());
    } else {
        return static_cast<unsigned long long>(left.nsFromOrigin() - right.nsFromOrigin());
    }
}

} // namespace internal

static inline Duration operator-(const Ts& left, const Ts& right)
{
    assert(left >= right);

    return Duration {internal::diff(left, right)};
}

} // namespace jacques

#endif // _JACQUES_DATA_TIME_OPS_HPP
