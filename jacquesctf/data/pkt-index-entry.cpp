/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "pkt-index-entry.hpp"

namespace jacques {

PktIndexEntry::PktIndexEntry(const Index indexInDsFile, const Index offsetInDsFileBytes,
                             boost::optional<Size> pktCtxOffsetInPktBits,
                             boost::optional<DataLen> preambleLen,
                             boost::optional<DataLen> expectedTotalLen,
                             boost::optional<DataLen> expectedContentLen,
                             const DataLen& effectiveTotalLen, const DataLen& effectiveContentLen,
                             const yactfr::DataStreamType *dst, boost::optional<Index> dsId,
                             boost::optional<Ts> beginTs, boost::optional<Ts> endTs,
                             boost::optional<Index> seqNum,
                             boost::optional<Size> discardedErCounter, const bool isInvalid) noexcept :
    _indexInDsFile {indexInDsFile},
    _offsetInDsFileBytes {offsetInDsFileBytes},
    _pktCtxOffsetInPktBits {std::move(pktCtxOffsetInPktBits)},
    _preambleLen {std::move(preambleLen)},
    _expectedTotalLen {std::move(expectedTotalLen)},
    _expectedContentLen {std::move(expectedContentLen)},
    _effectiveTotalLen {std::move(effectiveTotalLen)},
    _effectiveContentLen {std::move(effectiveContentLen)},
    _dst {dst},
    _dsId {std::move(dsId)},
    _beginTs {std::move(beginTs)},
    _endTs {std::move(endTs)},
    _seqNum {std::move(seqNum)},
    _discardedErCounter {std::move(discardedErCounter)},
    _isInvalid {isInvalid}
{
}

} // namespace jacques
