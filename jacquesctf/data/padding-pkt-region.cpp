/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "padding-pkt-region.hpp"

namespace jacques {

PaddingPktRegion::PaddingPktRegion(const PktSegment& segment,
                                         Scope::SP scope) :
    PktRegion {segment, std::move(scope)}
{
}

void PaddingPktRegion::_accept(PktRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
