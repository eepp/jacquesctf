/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_SCOPE_HPP
#define _JACQUES_DATA_SCOPE_HPP

#include <memory>
#include <yactfr/metadata/fwd.hpp>
#include <boost/core/noncopyable.hpp>

#include "data/event-record.hpp"
#include "packet-segment.hpp"

namespace jacques {

class Scope :
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<Scope>;
    using SPC = std::shared_ptr<const Scope>;

public:
    explicit Scope(yactfr::Scope scope);
    explicit Scope(EventRecord::SP eventRecord, yactfr::Scope scope);
    explicit Scope(EventRecord::SP eventRecord, yactfr::Scope scope,
                   const PacketSegment& segment);

    const EventRecord *eventRecord() const noexcept
    {
        return _eventRecord.get();
    }

    EventRecord::SP eventRecordPtr()
    {
        return _eventRecord;
    }

    EventRecord::SPC eventRecordPtr() const
    {
        return _eventRecord;
    }

    yactfr::Scope scope() const noexcept
    {
        return _scope;
    }

    const yactfr::DataType *dataType() const noexcept
    {
        return _dataType;
    }

    void dataType(const yactfr::DataType& dataType) noexcept
    {
        _dataType = &dataType;
    }

    const PacketSegment& segment() const noexcept
    {
        return _segment;
    }

    PacketSegment& segment() noexcept
    {
        return _segment;
    }

    void segment(const PacketSegment& segment) noexcept
    {
        _segment = segment;
    }

private:
    EventRecord::SP _eventRecord;
    const yactfr::Scope _scope;
    const yactfr::DataType *_dataType = nullptr;
    PacketSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_DATA_SCOPE_HPP
