/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <yactfr/metadata/event-record-type.hpp>
#include <yactfr/decoding-errors.hpp>

#include "data/event-record.hpp"

namespace jacques {

EventRecord::EventRecord(const yactfr::EventRecordType& type,
                         const Index indexInPacket,
                         const boost::optional<Timestamp>& firstTs,
                         const PacketSegment& segment) :
    _type {&type},
    _indexInPacket {indexInPacket},
    _firstTs {firstTs},
    _segment {segment}
{
}

EventRecord::EventRecord(Index indexInPacket) :
    _indexInPacket {indexInPacket}
{
}

EventRecord::SP EventRecord::createFromElementSequenceIterator(yactfr::ElementSequenceIterator& it,
                                                              const Metadata& metadata,
                                                              const Index packetOffsetInDataStreamFileBytes,
                                                              const Index indexInPacket)
{
    const auto packetOffsetInDataStreamFileBits = packetOffsetInDataStreamFileBytes * 8;
    EventRecord::SP eventRecord;

    try {
        while (true) {
            // TODO: replace with element visitor
            switch (it->kind()) {
            case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
                eventRecord = std::make_shared<EventRecord>(indexInPacket);
                eventRecord->segment().offsetInPacketBits(it.offset() - packetOffsetInDataStreamFileBits);
                break;

            case yactfr::Element::Kind::EVENT_RECORD_TYPE:
            {
                auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*it);

                eventRecord->type(elem.eventRecordType());
                break;
            }

            case yactfr::Element::Kind::CLOCK_VALUE:
            {
                if (eventRecord->firstTimestamp() || !metadata.isCorrelatable()) {
                    break;
                }

                auto& elem = static_cast<const yactfr::ClockValueElement&>(*it);

                eventRecord->firstTimestamp(Timestamp {elem});
                break;
            }

            case yactfr::Element::Kind::EVENT_RECORD_END:
            {
                eventRecord->segment().size(it.offset() - packetOffsetInDataStreamFileBits -
                                            eventRecord->segment().offsetInPacketBits());
                return eventRecord;
            }

            default:
                break;
            }

            ++it;
        }
    } catch (const yactfr::DecodingError& ex) {
        return eventRecord;
    }

    std::abort();
    return nullptr;
}

} // namespace jacques
