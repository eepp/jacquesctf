/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_TIME_OPS_HPP
#define _JACQUES_DATA_TIME_OPS_HPP

#include <cassert>

#include "ts.hpp"
#include "duration.hpp"

namespace jacques {

inline Duration operator-(const Ts& left, const Ts& right) noexcept
{
    assert(left >= right);

    // exact since `left >= right`, even if the signed difference overflows
    return Duration {static_cast<unsigned long long>(left.nsFromOrigin()) -
                     static_cast<unsigned long long>(right.nsFromOrigin())};
}

} // namespace jacques

#endif // _JACQUES_DATA_TIME_OPS_HPP
