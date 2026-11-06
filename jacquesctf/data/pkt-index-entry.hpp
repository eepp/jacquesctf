/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_PKT_INDEX_ENTRY_HPP
#define _JACQUES_DATA_PKT_INDEX_ENTRY_HPP

#include <optional>
#include <boost/operators.hpp>
#include <yactfr/yactfr.hpp>

#include "aliases.hpp"
#include "ts.hpp"
#include "data-len.hpp"

namespace jacques {

class PktIndexEntry final :
    public boost::totally_ordered<PktIndexEntry>
{
public:
    explicit PktIndexEntry(Index indexInDsFile, Index offsetInDsFileBytes,
                           std::optional<Index> pktCtxOffsetInPktBits,
                           std::optional<DataLen> preambleLen,
                           std::optional<DataLen> expectedTotalLen,
                           std::optional<DataLen> expectedContentLen,
                           const DataLen& effectiveTotalLen, const DataLen& effectiveContentLen,
                           const yactfr::DataStreamType *dst, std::optional<Index> dsId,
                           std::optional<Ts> beginTs, std::optional<Ts> endTs,
                           std::optional<Index> seqNum, std::optional<Size> discErCounterSnap,
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

    const std::optional<DataLen>& preambleLen() const noexcept
    {
        return _preambleLen;
    }

    const std::optional<Index>& pktCtxOffsetInPktBits() const noexcept
    {
        return _pktCtxOffsetInPktBits;
    }

    const std::optional<DataLen>& expectedTotalLen() const noexcept
    {
        return _expectedTotalLen;
    }

    const std::optional<DataLen>& expectedContentLen() const noexcept
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

    const std::optional<Ts>& beginTs() const noexcept
    {
        return _beginTs;
    }

    const std::optional<Ts>& endTs() const noexcept
    {
        return _endTs;
    }

    const std::optional<Index>& seqNum() const noexcept
    {
        return _seqNum;
    }

    const std::optional<Index>& dsId() const noexcept
    {
        return _dsId;
    }

    const std::optional<Size>& discErCounterSnap() const noexcept
    {
        return _discErCounterSnap;
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

    const std::optional<Size>& erCount() const noexcept
    {
        return _erCount;
    }

    void erCount(const std::optional<Size>& erCount) noexcept
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
    const std::optional<Index> _pktCtxOffsetInPktBits;
    const std::optional<DataLen> _preambleLen;
    const std::optional<DataLen> _expectedTotalLen;
    const std::optional<DataLen> _expectedContentLen;
    const DataLen _effectiveTotalLen;
    const DataLen _effectiveContentLen;
    const yactfr::DataStreamType *_dst;
    const std::optional<Index> _dsId;
    const std::optional<Ts> _beginTs;
    const std::optional<Ts> _endTs;
    const std::optional<Index> _seqNum;
    const std::optional<Size> _discErCounterSnap;
    bool _isInvalid;
    std::optional<Size> _erCount;
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_INDEX_ENTRY_HPP
