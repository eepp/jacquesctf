/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "pkt-segment.hpp"

namespace jacques {

PktSegment::PktSegment(const Index offsetInPktBits, const DataLen& len, OptBo bo) noexcept :
    _offsetInPktBits {offsetInPktBits},
    _len {len},
    _bo {std::move(bo)}
{
}

} // namespace jacques
