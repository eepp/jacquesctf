/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMON_PKT_STATE_HPP
#define _JACQUES_INSPECT_COMMON_PKT_STATE_HPP

#include "data/pkt.hpp"
#include "aliases.hpp"

namespace jacques {

class AppState;

class PktState final
{
public:
    explicit PktState(AppState& appState, const Metadata& metadata, Pkt& pkt) noexcept;

    PktState(const PktState&) = delete;
    PktState& operator=(const PktState&) = delete;

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

    AppState& appState() noexcept
    {
        return *_appState;
    }

    const AppState& appState() const noexcept
    {
        return *_appState;
    }

private:
    AppState *_appState;
    const Metadata *_metadata;
    Pkt *_pkt;
    Index _curOffsetInPktBits = 0;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMON_PKT_STATE_HPP
