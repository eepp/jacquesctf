/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
