/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_EVENT_RECORD_HPP
#define _JACQUES_EVENT_RECORD_HPP

#include <memory>
#include <boost/optional.hpp>
#include <boost/operators.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/packet-sequence-iterator.hpp>

#include "aliases.hpp"
#include "data-segment.hpp"
#include "timestamp.hpp"
#include "metadata.hpp"

namespace jacques {

class EventRecord :
    public boost::totally_ordered<EventRecord>
{
public:
    using SP = std::shared_ptr<const EventRecord>;

public:
    static SP createFromPacketSequenceIterator(yactfr::PacketSequenceIterator& it,
                                               const Metadata& metadata,
                                               Index packetOffsetInDataStreamBytes,
                                               Index indexInPacket);

public:
    explicit EventRecord(const yactfr::EventRecordType& type,
                         Index indexInPacket,
                         boost::optional<Timestamp> firstTs,
                         const DataSegment& segment);

    const yactfr::EventRecordType& type() const noexcept
    {
        return *_type;
    }

    Index indexInPacket() const noexcept
    {
        return _indexInPacket;
    }

    Index natIndexInPacket() const noexcept
    {
        return _indexInPacket + 1;
    }

    const DataSegment& segment() const noexcept
    {
        return _segment;
    }

    const boost::optional<Timestamp>& firstTimestamp() const noexcept
    {
        return _firstTs;
    }

    bool operator<(const EventRecord& other) const noexcept
    {
        return _indexInPacket < other._indexInPacket;
    }

    bool operator==(const EventRecord& other) const noexcept
    {
        return _indexInPacket == other._indexInPacket;
    }

private:
    const yactfr::EventRecordType *_type;
    Index _indexInPacket;
    boost::optional<Timestamp> _firstTs;
    DataSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_EVENT_RECORD_HPP
