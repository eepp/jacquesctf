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

#include "timestamp.hpp"
#include "duration.hpp"

namespace jacques {

static inline
Duration operator-(const Timestamp& left, const Timestamp& right)
{
    assert(left >= right);

    unsigned long long diff;

    if (left.nsFromOrigin() >= 0 && right.nsFromOrigin() < 0) {
        diff = static_cast<unsigned long long>(left.nsFromOrigin()) +
               static_cast<unsigned long long>(-right.nsFromOrigin());
    } else {
        diff = static_cast<unsigned long long>(left.nsFromOrigin() -
                                               right.nsFromOrigin());
    }

    return Duration {diff};
}

} // namespace jacques

#endif // _JACQUES_DATA_TIME_OPS_HPP
