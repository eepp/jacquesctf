/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <yactfr/yactfr.hpp>

#include "er.hpp"

namespace jacques {

Er::Er(const yactfr::EventRecordType& type, const Index indexInPkt,
       boost::optional<Ts> ts, const PktSegment& segment) noexcept :
    _type {&type},
    _indexInPkt {indexInPkt},
    _ts {std::move(ts)},
    _segment {segment}
{
}

Er::Er(Index indexInPkt) noexcept :
    _indexInPkt {indexInPkt}
{
}

Er::SP Er::createFromElemSeqIt(yactfr::ElementSequenceIterator& it, const Metadata& metadata,
                               const PktIndexEntry& pktIndexEntry, const Index indexInPkt)
{
    const auto pktOffsetInDsFileBits = pktIndexEntry.offsetInDsFileBytes() * 8;
    Er::SP er;
    boost::optional<Ts> curTs;

    try {
        while (true) {
            // TODO: replace with element visitor
            switch (it->kind()) {
            case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
                er = std::make_shared<Er>(indexInPkt);
                er->segment().offsetInPktBits(it.offset() - pktOffsetInDsFileBits);
                break;

            case yactfr::Element::Kind::EVENT_RECORD_INFO:
            {
                auto& elem = it->asEventRecordInfoElement();

                if (elem.type()) {
                    er->type(*elem.type());
                }

                if (curTs && metadata.isCorrelatable()) {
                    er->ts(*curTs);
                }

                break;
            }

            case yactfr::Element::Kind::DEFAULT_CLOCK_VALUE:
            {
                if (metadata.isCorrelatable()) {
                    auto& elem = it->asDefaultClockValueElement();

                    assert(pktIndexEntry.dst());
                    assert(pktIndexEntry.dst()->defaultClockType());
                    curTs = Ts {elem.cycles(), *pktIndexEntry.dst()->defaultClockType()};
                }

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
