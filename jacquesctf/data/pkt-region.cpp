/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "pkt-region.hpp"

namespace jacques {

PktRegion::PktRegion(const PktSegment& segment, Scope::SP scope) noexcept :
    _seg {segment},
    _scope {std::move(scope)}
{
}

} // namespace jacques
