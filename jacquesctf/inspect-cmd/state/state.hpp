/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_STATE_HPP
#define _JACQUES_INSPECT_CMD_STATE_STATE_HPP

#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/core/noncopyable.hpp>

#include "msg.hpp"
#include "ds-file-state.hpp"
#include "search-query.hpp"
#include "data/pkt-checkpoints-build-listener.hpp"
#include "data/trace.hpp"

namespace jacques {

/*
 * This state (application model) guarantees the following:
 *
 * * There is at least one data stream file state.
 *
 * * All data stream file state paths are unique.
 *
 * * There is always an active data stream file state.
 *
 * * All the contained data stream file states have a valid metadata
 *   object, because State::State() throws when there's any
 *   stream/parsing error.
 *
 * However, note that it's possible that any data stream file state,
 * including the active one, has no packets at all. This is only true
 * when the data stream file is empty, otherwise there's always at least
 * one available packet, but it could contain a decoding error.
 */
class State final :
    boost::noncopyable
{
    friend class DsFileState;
    friend class PktState;

public:
    using Observer = std::function<void (Message)>;

public:
    explicit State(const std::vector<boost::filesystem::path>& paths,
                   std::shared_ptr<PktCheckpointsBuildListener> pktCheckpointsBuildListener);
    Index addObserver(const Observer& observer);
    void removeObserver(Index id);
    void gotoDsFile(Index index);
    void gotoPrevDsFile();
    void gotoNextDsFile();
    bool search(const SearchQuery& query);

    DsFileState& activeDsFileState() const noexcept
    {
        return *_activeDsFileState;
    }

    Index activeDsFileStateIndex() const noexcept
    {
        return _activeDsFileStateIndex;
    }

    Size dsFileStateCount() const noexcept
    {
        return _dsFileStates.size();
    }

    const Metadata& metadata() const noexcept
    {
        return _activeDsFileState->metadata();
    }

    void gotoPkt(const Index index)
    {
        _activeDsFileState->gotoPkt(index);
    }

    void gotoPrevPkt()
    {
        _activeDsFileState->gotoPrevPkt();
    }

    void gotoNextPkt()
    {
        _activeDsFileState->gotoNextPkt();
    }

    void gotoPrevEr(const Size count = 1)
    {
        _activeDsFileState->gotoPrevEr(count);
    }

    void gotoNextEr(const Size count = 1)
    {
        _activeDsFileState->gotoNextEr(count);
    }

    void gotoPrevPktRegion()
    {
        _activeDsFileState->gotoPrevPktRegion();
    }

    void gotoNextPktRegion()
    {
        _activeDsFileState->gotoNextPktRegion();
    }

    void gotoPktCtx()
    {
        _activeDsFileState->gotoPktCtx();
    }

    void gotoLastPktRegion()
    {
        _activeDsFileState->gotoLastPktRegion();
    }

    bool hasActivePktState() const noexcept
    {
        return _activeDsFileState->hasActivePktState();
    }

    PktState& activePktState()
    {
        return _activeDsFileState->activePktState();
    }

    Index curOffsetInPktBits() const noexcept
    {
        return _activeDsFileState->curOffsetInPktBits();
    }

    void gotoPktRegionAtOffsetInPktBits(const Index offsetInPktBits)
    {
        _activeDsFileState->gotoPktRegionAtOffsetInPktBits(offsetInPktBits);
    }

    const Er *curEr()
    {
        return _activeDsFileState->curEr();
    }

    const PktRegion *curPktRegion()
    {
        return _activeDsFileState->curPktRegion();
    }

    const DsFileState& dsFileState(const Index index) const noexcept
    {
        return *_dsFileStates[index];
    }

    std::vector<std::unique_ptr<DsFileState>>& dsFileStates()
    {
        return _dsFileStates;
    }

    const std::vector<std::unique_ptr<DsFileState>>& dsFileStates() const
    {
        return _dsFileStates;
    }

private:
    void _notify(Message msg);

private:
    std::vector<Observer> _observers;
    std::vector<std::unique_ptr<DsFileState>> _dsFileStates;
    DsFileState *_activeDsFileState;
    Index _activeDsFileStateIndex = 0;
    std::vector<std::unique_ptr<Trace>> _traces;
};

class StateObserverGuard final
{
public:
    explicit StateObserverGuard(State& state, const State::Observer& observer);
    ~StateObserverGuard();

private:
    State * const _state;
    const Index _observerId;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_STATE_HPP
