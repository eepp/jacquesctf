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
#include <boost/core/noncopyable.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/packet-sequence-iterator.hpp>

#include "aliases.hpp"
#include "packet-segment.hpp"
#include "timestamp.hpp"
#include "metadata.hpp"

namespace jacques {

class EventRecord :
    public boost::totally_ordered<EventRecord>,
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<EventRecord>;
    using SPC = std::shared_ptr<const EventRecord>;

public:
    static SP createFromPacketSequenceIterator(yactfr::PacketSequenceIterator& it,
                                               const Metadata& metadata,
                                               Index packetOffsetInDataStreamBytes,
                                               Index indexInPacket);

public:
    explicit EventRecord(Index indexInPacket);
    explicit EventRecord(const yactfr::EventRecordType& type,
                         const Index indexInPacket,
                         const boost::optional<Timestamp>& firstTs,
                         const PacketSegment& segment);

    const yactfr::EventRecordType *type() const noexcept
    {
        return _type;
    }

    void type(const yactfr::EventRecordType& type) noexcept
    {
        _type = &type;
    }

    Index indexInPacket() const noexcept
    {
        return _indexInPacket;
    }

    Index natIndexInPacket() const noexcept
    {
        return _indexInPacket + 1;
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

    const boost::optional<Timestamp>& firstTimestamp() const noexcept
    {
        return _firstTs;
    }

    void firstTimestamp(const Timestamp& firstTs) noexcept
    {
        _firstTs = firstTs;
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
    const yactfr::EventRecordType *_type = nullptr;
    Index _indexInPacket;
    boost::optional<Timestamp> _firstTs;
    PacketSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_EVENT_RECORD_HPP
