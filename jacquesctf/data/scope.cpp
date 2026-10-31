/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "scope.hpp"

namespace jacques {

Scope::Scope(Er::SP er, const yactfr::Scope scope, const PktSegment& segment) noexcept :
    _er {std::move(er)},
    _scope {scope},
    _segment {segment}
{
}

Scope::Scope(const yactfr::Scope scope) noexcept :
    Scope {nullptr, scope, PktSegment {}}
{
}

} // namespace jacques
