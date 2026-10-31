/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "padding-pkt-region.hpp"

namespace jacques {

PaddingPktRegion::PaddingPktRegion(const PktSegment& segment, Scope::SP scope) :
    PktRegion {segment, std::move(scope)}
{
}

void PaddingPktRegion::_accept(PktRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
