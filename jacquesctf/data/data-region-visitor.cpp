/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-region-visitor.hpp"

namespace jacques {

DataRegionVisitor::~DataRegionVisitor()
{
}

void DataRegionVisitor::visit(const ContentDataRegion&)
{
}

void DataRegionVisitor::visit(const PaddingDataRegion&)
{
}

void DataRegionVisitor::visit(const ErrorDataRegion&)
{
}

} // namespace jacques
