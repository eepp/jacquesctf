/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "pkt-region-visitor.hpp"

namespace jacques {

void PktRegionVisitor::visit(const ContentPktRegion&)
{
}

void PktRegionVisitor::visit(const PaddingPktRegion&)
{
}

void PktRegionVisitor::visit(const ErrorPktRegion&)
{
}

} // namespace jacques
