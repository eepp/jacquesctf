/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "packet-region.hpp"

namespace jacques {

PacketRegion::PacketRegion(const PacketSegment& segment, Scope::SP scope) :
    _seg {segment},
    _scope {std::move(scope)}
{
}

PacketRegion::~PacketRegion()
{
}

} // namespace jacques
