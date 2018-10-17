/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "packet-index-entry.hpp"

namespace jacques {

PacketIndexEntry::PacketIndexEntry(const Index indexInDataStream,
                                   const Index offsetInDataStreamBytes,
                                   const boost::optional<Size> packetContextOffsetInPacketBits,
                                   const boost::optional<DataSize>& preambleSize,
                                   const boost::optional<DataSize>& expectedTotalSize,
                                   const boost::optional<DataSize>& expectedContentSize,
                                   const DataSize& effectiveTotalSize,
                                   const DataSize& effectiveContentSize,                                   const yactfr::DataStreamType *dst,
                                   const boost::optional<Index> dataStreamId,
                                   const boost::optional<Timestamp> beginningTimestamp,
                                   const boost::optional<Timestamp> endTimestamp,
                                   const boost::optional<Index> seqNum,
                                   const boost::optional<Size> discardedEventRecordCounter,
                                   const bool isInvalid) :
    _indexInDataStream {indexInDataStream},
    _offsetInDataStreamBytes {offsetInDataStreamBytes},
    _packetContextOffsetInPacketBits {packetContextOffsetInPacketBits},
    _preambleSize {preambleSize},
    _expectedTotalSize {expectedTotalSize},
    _expectedContentSize {expectedContentSize},
    _effectiveTotalSize {effectiveTotalSize},
    _effectiveContentSize {effectiveContentSize},
    _dst {dst},
    _dataStreamId {dataStreamId},
    _beginningTimestamp {beginningTimestamp},
    _endTimestamp {endTimestamp},
    _seqNum {seqNum},
    _discardedEventRecordCounter {discardedEventRecordCounter},
    _isInvalid {isInvalid}
{
}

} // namespace jacques
