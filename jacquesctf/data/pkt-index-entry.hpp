/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_PKT_INDEX_ENTRY_HPP
#define _JACQUES_DATA_PKT_INDEX_ENTRY_HPP

#include <boost/optional.hpp>
#include <boost/operators.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "aliases.hpp"
#include "ts.hpp"
#include "data-len.hpp"

namespace jacques {

class PktIndexEntry :
    public boost::totally_ordered<PktIndexEntry>
{
public:
    explicit PktIndexEntry(Index indexInDsFile, Index offsetInDsFileBytes,
                           boost::optional<Index> pktCtxOffsetInPktBits,
                           boost::optional<DataLen> preambleLen,
                           boost::optional<DataLen> expectedTotalLen,
                           boost::optional<DataLen> expectedContentLen,
                           const DataLen& effectiveTotalLen, const DataLen& effectiveContentLen,
                           const yactfr::DataStreamType *dst, boost::optional<Index> dsId,
                           boost::optional<Ts> beginTs, boost::optional<Ts> endTs,
                           boost::optional<Index> seqNum, boost::optional<Size> discardedErCounter,
                           bool isInvalid) noexcept;

    PktIndexEntry(const PktIndexEntry&) = default;

    Index offsetInDsFileBytes() const noexcept
    {
        return _offsetInDsFileBytes;
    }

    Index offsetInDsFileBits() const noexcept
    {
        return _offsetInDsFileBytes * 8;
    }

    Index endOffsetInDsFileBytes() const noexcept
    {
        return _offsetInDsFileBytes + _effectiveTotalLen.bytes();
    }

    Index endOffsetInDsFileBits() const noexcept
    {
        return this->endOffsetInDsFileBytes() * 8;
    }

    const boost::optional<DataLen>& preambleLen() const noexcept
    {
        return _preambleLen;
    }

    const boost::optional<Index>& pktCtxOffsetInPktBits() const noexcept
    {
        return _pktCtxOffsetInPktBits;
    }

    const boost::optional<DataLen>& expectedTotalLen() const noexcept
    {
        return _expectedTotalLen;
    }

    const boost::optional<DataLen>& expectedContentLen() const noexcept
    {
        return _expectedContentLen;
    }

    const DataLen& effectiveTotalLen() const noexcept
    {
        return _effectiveTotalLen;
    }

    const DataLen& effectiveContentLen() const noexcept
    {
        return _effectiveContentLen;
    }

    const boost::optional<Ts>& beginTs() const noexcept
    {
        return _beginTs;
    }

    const boost::optional<Ts>& endTs() const noexcept
    {
        return _endTs;
    }

    const boost::optional<Index>& seqNum() const noexcept
    {
        return _seqNum;
    }

    const boost::optional<Index>& dsId() const noexcept
    {
        return _dsId;
    }

    const boost::optional<Size>& discardedErCounter() const noexcept
    {
        return _discardedErCounter;
    }

    Index indexInDsFile() const noexcept
    {
        return _indexInDsFile;
    }

    Index natIndexInDsFile() const noexcept
    {
        return _indexInDsFile + 1;
    }

    // can be `nullptr` if this entry is invalid
    const yactfr::DataStreamType *dst() const noexcept
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

    const boost::optional<Size>& erCount() const noexcept
    {
        return _erCount;
    }

    void erCount(const boost::optional<Size>& erCount) noexcept
    {
        _erCount = erCount;
    }

    bool operator<(const PktIndexEntry& other) const noexcept
    {
        return _indexInDsFile < other._indexInDsFile;
    }

    bool operator==(const PktIndexEntry& other) const noexcept
    {
        return _indexInDsFile == other._indexInDsFile;
    }

private:
    const Index _indexInDsFile;
    const Size _offsetInDsFileBytes;
    const boost::optional<Index> _pktCtxOffsetInPktBits;
    const boost::optional<DataLen> _preambleLen;
    const boost::optional<DataLen> _expectedTotalLen;
    const boost::optional<DataLen> _expectedContentLen;
    const DataLen _effectiveTotalLen;
    const DataLen _effectiveContentLen;
    const yactfr::DataStreamType *_dst;
    const boost::optional<Index> _dsId;
    const boost::optional<Ts> _beginTs;
    const boost::optional<Ts> _endTs;
    const boost::optional<Index> _seqNum;
    const boost::optional<Size> _discardedErCounter;
    bool _isInvalid;
    boost::optional<Size> _erCount;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_INDEX_ENTRY_HPP
