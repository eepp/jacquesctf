/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <yactfr/metadata/event-record-type.hpp>
#include <yactfr/decoding-errors.hpp>

#include "er.hpp"

namespace jacques {

Er::Er(const yactfr::EventRecordType& type, const Index indexInPkt,
       boost::optional<Ts> firstTs, const PktSegment& segment) noexcept :
    _type {&type},
    _indexInPkt {indexInPkt},
    _firstTs {std::move(firstTs)},
    _segment {segment}
{
}

Er::Er(Index indexInPkt) noexcept :
    _indexInPkt {indexInPkt}
{
}

Er::SP Er::createFromElemSeqIt(yactfr::ElementSequenceIterator& it, const Metadata& metadata,
                               const Index pktOffsetInDsFileBytes, const Index indexInPkt)
{
    const auto pktOffsetInDsFileBits = pktOffsetInDsFileBytes * 8;
    Er::SP er;

    try {
        while (true) {
            // TODO: replace with element visitor
            switch (it->kind()) {
            case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
                er = std::make_shared<Er>(indexInPkt);
                er->segment().offsetInPktBits(it.offset() - pktOffsetInDsFileBits);
                break;

            case yactfr::Element::Kind::EVENT_RECORD_TYPE:
            {
                auto& elem = static_cast<const yactfr::EventRecordTypeElement&>(*it);

                er->type(elem.eventRecordType());
                break;
            }

            case yactfr::Element::Kind::CLOCK_VALUE:
            {
                if (er->firstTs() || !metadata.isCorrelatable()) {
                    break;
                }

                auto& elem = static_cast<const yactfr::ClockValueElement&>(*it);

                er->firstTs(Ts {elem});
                break;
            }

            case yactfr::Element::Kind::EVENT_RECORD_END:
            {
                er->segment().len(it.offset() - pktOffsetInDsFileBits -
                                  er->segment().offsetInPktBits());
                return er;
            }

            default:
                break;
            }

            ++it;
        }
    } catch (const yactfr::DecodingError&) {
        return er;
    }

    std::abort();
    return nullptr;
}

} // namespace jacques
