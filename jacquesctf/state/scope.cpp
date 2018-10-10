/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "scope.hpp"

namespace jacques {

Scope::Scope(EventRecord::SP eventRecord, yactfr::Scope scope,
             const yactfr::DataType& rootDataType, const DataSegment& segment) :
    _eventRecord {eventRecord},
    _scope {scope},
    _rootDataType {&rootDataType},
    _segment {segment}
{
}

} // namespace jacques
