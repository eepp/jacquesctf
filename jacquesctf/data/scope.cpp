/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
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
