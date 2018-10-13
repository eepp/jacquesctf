/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "scope.hpp"

namespace jacques {

Scope::Scope(EventRecord::SP eventRecord, const yactfr::Scope scope,
             const DataSegment& segment) :
    _eventRecord {std::move(eventRecord)},
    _scope {scope},
    _segment {segment}
{
}

Scope::Scope(EventRecord::SP eventRecord, const yactfr::Scope scope) :
    Scope {std::move(eventRecord), scope, {}}
{
}

Scope::Scope(const yactfr::Scope scope) :
    Scope {nullptr, scope, {}}
{
}

} // namespace jacques
