/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_TIME_OPS_HPP
#define _JACQUES_TIME_OPS_HPP

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
    const auto diff = left.nsFromEpoch() - right.nsFromEpoch();

    assert(diff >= 0);
    return Duration {static_cast<unsigned long long>(diff)};
}

} // namespace jacques

#endif // _JACQUES_TIME_OPS_HPP
