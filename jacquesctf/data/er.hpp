/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_ER_HPP
#define _JACQUES_DATA_ER_HPP

#include <memory>
#include <optional>
#include <boost/operators.hpp>

#include "aliases.hpp"
#include "pkt-segment.hpp"
#include "ts.hpp"
#include "pkt-index-entry.hpp"

namespace jacques {

class Er final :
    public boost::totally_ordered<Er>
{
public:
    using SP = std::shared_ptr<Er>;
    using SPC = std::shared_ptr<const Er>;

public:
    [[ nodiscard ]] static SP createFromElemSeqIt(yactfr::ElementSequenceIterator& it,
                                                  const PktIndexEntry& pktIndexEntry, Index indexInPkt);

public:
    explicit Er(Index indexInPkt) noexcept;

    explicit Er(const yactfr::EventRecordType& type, const Index indexInPkt,
                std::optional<Ts> ts, const PktSegment& segment) noexcept;

    Er(const Er&) = delete;
    Er& operator=(const Er&) = delete;

    const yactfr::EventRecordType *type() const noexcept
    {
        return _type;
    }

    void type(const yactfr::EventRecordType& type) noexcept
    {
        _type = &type;
    }

    Index indexInPkt() const noexcept
    {
        return _indexInPkt;
    }

    Index natIndexInPkt() const noexcept
    {
        return _indexInPkt + 1;
    }

    const PktSegment& segment() const noexcept
    {
        return _segment;
    }

    PktSegment& segment() noexcept
    {
        return _segment;
    }

    void segment(const PktSegment& segment) noexcept
    {
        _segment = segment;
    }

    const std::optional<Ts>& ts() const noexcept
    {
        return _ts;
    }

    void ts(const Ts& ts) noexcept
    {
        _ts = ts;
    }

    bool operator<(const Er& other) const noexcept
    {
        return _indexInPkt < other._indexInPkt;
    }

    bool operator==(const Er& other) const noexcept
    {
        return _indexInPkt == other._indexInPkt;
    }

private:
    const yactfr::EventRecordType *_type = nullptr;
    Index _indexInPkt;
    std::optional<Ts> _ts;
    PktSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_DATA_ER_HPP
