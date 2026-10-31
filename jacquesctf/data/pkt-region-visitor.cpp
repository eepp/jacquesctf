/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
