/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_DS_FILE_STATE_HPP
#define _JACQUES_INSPECT_CMD_STATE_DS_FILE_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <yactfr/yactfr.hpp>
#include <boost/core/noncopyable.hpp>

#include "aliases.hpp"
#include "data/ts.hpp"
#include "data/ds-file.hpp"
#include "search-query.hpp"
#include "data/pkt.hpp"
#include "data/er.hpp"
#include "data/metadata.hpp"
#include "pkt-state.hpp"

namespace jacques {

class State;

class DsFileState final :
    boost::noncopyable
{
public:
    explicit DsFileState(State& state, DsFile& dsFile,
                         std::shared_ptr<PktCheckpointsBuildListener> pktCheckpointsBuildListener);

    void gotoOffsetBits(Index offsetBits);
    void gotoPkt(Index index);
    void gotoPrevPkt();
    void gotoNextPkt();
    void gotoPrevEr(Size count = 1);
    void gotoNextEr(Size count = 1);
    void gotoPrevPktRegion();
    void gotoNextPktRegion();
    void gotoPktCtx();
    void gotoLastPktRegion();
    bool search(const SearchQuery& query);
    void analyzeAllPkts(PktCheckpointsBuildListener& buildListener);

    DsFile& dsFile() noexcept
    {
        return *_dsFile;
    }

    const DsFile& dsFile() const noexcept
    {
        return *_dsFile;
    }

    void gotoOffsetBytes(Index offsetBytes)
    {
        return this->gotoOffsetBits(offsetBytes * 8);
    }

    bool hasActivePktState() const noexcept
    {
        return _activePktState != nullptr;
    }

    PktState& activePktState()
    {
        return this->_pktState(_activePktStateIndex);
    }

    Index activePktStateIndex() const noexcept
    {
        return _activePktStateIndex;
    }

    Index curOffsetInPktBits() const noexcept
    {
        if (!_activePktState) {
            return 0;
        }

        return _activePktState->curOffsetInPktBits();
    }

    Index curOffsetInDsFileBits() const noexcept
    {
        if (!_activePktState) {
            return 0;
        }

        return _activePktState->pktIndexEntry().offsetInDsFileBits() +
               _activePktState->curOffsetInPktBits();
    }

    void gotoPktRegionAtOffsetInPktBits(const Index offsetInPktBits)
    {
        if (!_activePktState) {
            return;
        }

        _activePktState->gotoPktRegionAtOffsetInPktBits(offsetInPktBits);
    }

    const Er *curEr()
    {
        if (!_activePktState) {
            return nullptr;
        }

        return _activePktState->curEr();
    }

    const PktRegion *curPktRegion()
    {
        if (!_activePktState) {
            return nullptr;
        }

        if (_activePktState->pktIndexEntry().effectiveTotalLen() == 0) {
            return nullptr;
        }

        return &_activePktState->curPktRegion();
    }

    const Metadata& metadata() const noexcept
    {
        return _dsFile->metadata();
    }

    const Trace& trace() const noexcept
    {
        return _dsFile->trace();
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
    PktState& _pktState(Index index);
    void _gotoPkt(Index index);

    bool _gotoNextErWithProp(const std::function<bool (const Er&)>& cmpFunc,
                             const boost::optional<Index>& initPktIndex = boost::none,
                             const boost::optional<Index>& initErIndex = boost::none);

private:
    State * const _state;
    PktState *_activePktState = nullptr;
    Index _activePktStateIndex = 0;
    std::vector<std::unique_ptr<PktState>> _pktStates;
    std::shared_ptr<PktCheckpointsBuildListener> _pktCheckpointsBuildListener;
    DsFile * const _dsFile;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_DS_FILE_STATE_HPP
