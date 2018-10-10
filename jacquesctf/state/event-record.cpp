/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <yactfr/metadata/event-record-type.hpp>

#include "event-record.hpp"

namespace jacques {

EventRecord::EventRecord(const yactfr::EventRecordType& type,
                         Index indexInPacket,
                         boost::optional<Timestamp> firstTs,
                         const DataSegment& segment) :
    _type {&type},
    _indexInPacket {indexInPacket},
    _firstTs {firstTs},
    _segment {segment}
{
}

EventRecord::SP EventRecord::createFromPacketSequenceIterator(yactfr::PacketSequenceIterator& it,
                                                              const Metadata& metadata,
                                                              const Index packetOffsetInDataStreamBytes,
                                                              const Index indexInPacket)
{
    const yactfr::EventRecordType *eventRecordType = nullptr;
    boost::optional<Timestamp> firstTs;
    DataSegment segment;
    const auto packetOffsetInDataStreamBits = packetOffsetInDataStreamBytes * 8;

    while (true) {
        // TODO: replace with element visitor
        switch (it->kind()) {
        case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
        {
            /*
             * Find the first structure to skip the padding before the
             * event record, so as to get an expected offset.
             */
            auto isDone = false;

            while (!isDone) {
                switch (it->kind()) {
                case yactfr::Element::Kind::STRUCT_BEGINNING:
                    segment.offsetInPacketBits(it.offset() - packetOffsetInDataStreamBits);
                    isDone = true;
                    break;

                case yactfr::Element::Kind::EVENT_RECORD_TYPE:
                {
                    auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*it);

                    eventRecordType = &elem.eventRecordType();
                    break;
                }

                default:
                    break;
                }

                ++it;
            }

            break;
        }

        case yactfr::Element::Kind::EVENT_RECORD_TYPE:
        {
            auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*it);

            eventRecordType = &elem.eventRecordType();
            break;
        }

        case yactfr::Element::Kind::CLOCK_VALUE:
        {
            if (firstTs || !metadata.isCorrelatable()) {
                break;
            }

            auto& elem = static_cast<const yactfr::ClockValueElement&>(*it);

            firstTs = Timestamp {elem};
            break;
        }

        case yactfr::Element::Kind::EVENT_RECORD_END:
        {
            segment.size(it.offset() - packetOffsetInDataStreamBits -
                         segment.offsetInPacketBits());
            assert(eventRecordType);
            return std::make_shared<EventRecord>(*eventRecordType,
                                                 indexInPacket,
                                                 firstTs, segment);
        }

        default:
            break;
        }

        ++it;
    }

    std::abort();
    return nullptr;
}

} // namespace jacques
