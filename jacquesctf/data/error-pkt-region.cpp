/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "error-pkt-region.hpp"

namespace jacques {

ErrorPktRegion::ErrorPktRegion(const PktSegment& segment) noexcept :
    PktRegion {segment}
{
}

void ErrorPktRegion::_accept(PktRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
