/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "packet-segment.hpp"

namespace jacques {

PacketSegment::PacketSegment(const Index offsetInPacketBits, const DataSize& size,
                             const OptByteOrder& byteOrder) :
    _offsetInPacketBits {offsetInPacketBits},
    _size {size},
    _byteOrder {byteOrder}
{
}

} // namespace jacques
