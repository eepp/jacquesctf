/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
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
