/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
