/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_PKT_STATE_HPP
#define _JACQUES_INSPECT_CMD_STATE_PKT_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/core/noncopyable.hpp>

#include "data/pkt.hpp"
#include "aliases.hpp"

namespace jacques {

class State;

class PktState final :
    boost::noncopyable
{
public:
    explicit PktState(State& state, const Metadata& metadata, Pkt& pkt) noexcept;
    void gotoPrevEr(Size count = 1);
    void gotoNextEr(Size count = 1);
    void gotoPrevPktRegion();
    void gotoNextPktRegion();
    void gotoPktCtx();
    void gotoLastPktRegion();
    void gotoPktRegionNextParent();
    void gotoPktRegionAtOffsetInPktBits(Index offsetBits);

    void gotoPktRegionAtOffsetInPktBits(const PktRegion& region)
    {
        this->gotoPktRegionAtOffsetInPktBits(region.segment().offsetInPktBits());
    }

    Pkt& pkt() noexcept
    {
        return *_pkt;
    }

    const PktIndexEntry& pktIndexEntry() const noexcept
    {
        return _pkt->indexEntry();
    }

    Index curOffsetInPktBits() const noexcept
    {
        return _curOffsetInPktBits;
    }

    const Er *curEr()
    {
        const auto& pktRegion = _pkt->regionAtOffsetInPktBits(_curOffsetInPktBits);
        const auto& scope = pktRegion.scope();

        if (!scope) {
            return nullptr;
        }

        // can return `nullptr`
        return scope->er();
    }

    const PktRegion& curPktRegion()
    {
        return _pkt->regionAtOffsetInPktBits(_curOffsetInPktBits);
    }

    State& state() noexcept
    {
        return *_state;
    }

    const State& state() const noexcept
    {
        return *_state;
    }

private:
    State * const _state;
    const Metadata * const _metadata;
    Pkt * const _pkt;
    Index _curOffsetInPktBits = 0;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_PKT_STATE_HPP
