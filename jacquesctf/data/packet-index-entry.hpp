/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PACKET_INDEX_ENTRY_HPP
#define _JACQUES_DATA_PACKET_INDEX_ENTRY_HPP

#include <boost/optional.hpp>
#include <boost/operators.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "aliases.hpp"
#include "timestamp.hpp"
#include "data-size.hpp"

namespace jacques {

class PacketIndexEntry :
    public boost::totally_ordered<PacketIndexEntry>
{
public:
    explicit PacketIndexEntry(Index indexInDataStreamFile,
                              Index offsetInDataStreamFileBytes,
                              boost::optional<Index> packetContextOffsetInPacketBits,
                              const boost::optional<DataSize>& preambleSize,
                              const boost::optional<DataSize>& expectedTotalSize,
                              const boost::optional<DataSize>& expectedContentSize,
                              const DataSize& effectiveTotalSize,
                              const DataSize& effectiveContentSize,
                              const yactfr::DataStreamType *dst,
                              boost::optional<Index> dataStreamId,
                              boost::optional<Timestamp> beginningTimestamp,
                              boost::optional<Timestamp> endTimestamp,
                              boost::optional<Index> seqNum,
                              boost::optional<Size> discardedEventRecordCounter,
                              bool isInvalid);
    PacketIndexEntry(const PacketIndexEntry&) = default;

    Index offsetInDataStreamFileBytes() const noexcept
    {
        return _offsetInDataStreamFileBytes;
    }

    Index offsetInDataStreamFileBits() const noexcept
    {
        return _offsetInDataStreamFileBytes * 8;
    }

    Index endOffsetInDataStreamFileBytes() const noexcept
    {
        return _offsetInDataStreamFileBytes + _effectiveTotalSize.bytes();
    }

    Index endOffsetInDataStreamFileBits() const noexcept
    {
        return this->endOffsetInDataStreamFileBytes() * 8;
    }

    const boost::optional<DataSize>& preambleSize() const noexcept
    {
        return _preambleSize;
    }

    const boost::optional<Index>& packetContextOffsetInPacketBits() const noexcept
    {
        return _packetContextOffsetInPacketBits;
    }

    const boost::optional<DataSize>& expectedTotalSize() const noexcept
    {
        return _expectedTotalSize;
    }

    const boost::optional<DataSize>& expectedContentSize() const noexcept
    {
        return _expectedContentSize;
    }

    const DataSize& effectiveTotalSize() const noexcept
    {
        return _effectiveTotalSize;
    }

    const DataSize& effectiveContentSize() const noexcept
    {
        return _effectiveContentSize;
    }

    const boost::optional<Timestamp>& beginningTimestamp() const noexcept
    {
        return _beginningTimestamp;
    }

    const boost::optional<Timestamp>& endTimestamp() const noexcept
    {
        return _endTimestamp;
    }

    const boost::optional<Index>& seqNum() const noexcept
    {
        return _seqNum;
    }

    const boost::optional<Index>& dataStreamId() const noexcept
    {
        return _dataStreamId;
    }

    const boost::optional<Size>& discardedEventRecordCounter() const noexcept
    {
        return _discardedEventRecordCounter;
    }

    Index indexInDataStreamFile() const noexcept
    {
        return _indexInDataStreamFile;
    }

    Index natIndexInDataStreamFile() const noexcept
    {
        return _indexInDataStreamFile + 1;
    }

    // can be `nullptr` if this entry is invalid
    const yactfr::DataStreamType *dataStreamType() const noexcept
    {
        return _dst;
    }

    bool isInvalid() const noexcept
    {
        return _isInvalid;
    }

    void isInvalid(const bool isInvalid) noexcept
    {
        _isInvalid = isInvalid;
    }

    const boost::optional<Size>& eventRecordCount() const noexcept
    {
        return _eventRecordCount;
    }

    void eventRecordCount(const boost::optional<Size>& eventRecordCount) noexcept
    {
        _eventRecordCount = eventRecordCount;
    }

    bool operator<(const PacketIndexEntry& other) const noexcept
    {
        return _indexInDataStreamFile < other._indexInDataStreamFile;
    }

    bool operator==(const PacketIndexEntry& other) const noexcept
    {
        return _indexInDataStreamFile == other._indexInDataStreamFile;
    }

private:
    const Index _indexInDataStreamFile;
    const Size _offsetInDataStreamFileBytes;
    const boost::optional<Index> _packetContextOffsetInPacketBits;
    const boost::optional<DataSize> _preambleSize;
    const boost::optional<DataSize> _expectedTotalSize;
    const boost::optional<DataSize> _expectedContentSize;
    const DataSize _effectiveTotalSize;
    const DataSize _effectiveContentSize;
    const yactfr::DataStreamType *_dst;
    const boost::optional<Index> _dataStreamId;
    const boost::optional<Timestamp> _beginningTimestamp;
    const boost::optional<Timestamp> _endTimestamp;
    const boost::optional<Index> _seqNum;
    const boost::optional<Size> _discardedEventRecordCounter;
    bool _isInvalid;
    boost::optional<Size> _eventRecordCount;
};

} // namespace jacques

#endif // _JACQUES_DATA_PACKET_INDEX_ENTRY_HPP
