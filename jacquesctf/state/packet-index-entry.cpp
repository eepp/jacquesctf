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
                                   const DataSize totalSize,
                                   const DataSize contentSize,
                                   const yactfr::DataStreamType *dst,
                                   const boost::optional<Index> dataStreamId,
                                   const boost::optional<Timestamp> tsBegin,
                                   const boost::optional<Timestamp> tsEnd,
                                   const boost::optional<Index> seqNum,
                                   const boost::optional<Size> discardedEventRecordCounter,
                                   const bool isInvalid) :
    _indexInDataStream {indexInDataStream},
    _offsetInDataStreamBytes {offsetInDataStreamBytes},
    _packetContextOffsetInPacketBits {packetContextOffsetInPacketBits},
    _totalSize {totalSize},
    _contentSize {contentSize},
    _dst {dst},
    _dataStreamId {dataStreamId},
    _tsBegin {tsBegin},
    _tsEnd {tsEnd},
    _seqNum {seqNum},
    _discardedEventRecordCounter {discardedEventRecordCounter},
    _isInvalid {isInvalid}
{
}

} // namespace jacques
