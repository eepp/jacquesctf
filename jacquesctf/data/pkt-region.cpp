/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "pkt-region.hpp"

namespace jacques {

PktRegion::PktRegion(const PktSegment& segment, Scope::SP scope) noexcept :
    _seg {segment},
    _scope {std::move(scope)}
{
}

} // namespace jacques
