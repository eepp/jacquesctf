/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_ER_HPP
#define _JACQUES_DATA_ER_HPP

#include <memory>
#include <boost/optional.hpp>
#include <boost/operators.hpp>
#include <boost/core/noncopyable.hpp>
#include <yactfr/yactfr.hpp>

#include "aliases.hpp"
#include "pkt-segment.hpp"
#include "ts.hpp"
#include "metadata.hpp"
#include "pkt-index-entry.hpp"

namespace jacques {

class Er final :
    public boost::totally_ordered<Er>,
    boost::noncopyable
{
public:
    using SP = std::shared_ptr<Er>;
    using SPC = std::shared_ptr<const Er>;

public:
    static SP createFromElemSeqIt(yactfr::ElementSequenceIterator& it, const Metadata& metadata,
                                  const PktIndexEntry& pktIndexEntry, Index indexInPkt);

public:
    explicit Er(Index indexInPkt) noexcept;

    explicit Er(const yactfr::EventRecordType& type, const Index indexInPkt,
                boost::optional<Ts> ts, const PktSegment& segment) noexcept;

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

    const boost::optional<Ts>& ts() const noexcept
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
    boost::optional<Ts> _ts;
    PktSegment _segment;
};

} // namespace jacques

#endif // _JACQUES_DATA_ER_HPP
