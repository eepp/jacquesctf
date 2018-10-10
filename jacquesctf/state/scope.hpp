/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_SCOPE_HPP
#define _JACQUES_SCOPE_HPP

#include <memory>
#include <yactfr/metadata/fwd.hpp>

#include "event-record.hpp"
#include "data-segment.hpp"

namespace jacques {

class Scope
{
public:
    using SP = std::shared_ptr<Scope>;

public:
    explicit Scope(EventRecord::SP eventRecord, yactfr::Scope scope,
                   const yactfr::DataType& rootDataType,
                   const DataSegment& segment);

    const EventRecord *eventRecord() const noexcept
    {
        return _eventRecord.get();
    }

    EventRecord::SP eventRecordPtr() const
    {
        return _eventRecord;
    }

    yactfr::Scope scope() const noexcept
    {
        return _scope;
    }

    const yactfr::DataType& rootDataType() const noexcept
    {
        return *_rootDataType;
    }

    const DataSegment& segment() const noexcept
    {
        return _segment;
    }

private:
    EventRecord::SP _eventRecord;
    yactfr::Scope _scope;
    const yactfr::DataType *_rootDataType;
    DataSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_SCOPE_HPP
