/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <utility>

#include "dt-path.hpp"

namespace jacques {

DtPath::DtPath(const yactfr::Scope scope, Items items) :
    _scope {scope},
    _items {std::move(items)}
{
}

} // namespace jacques
