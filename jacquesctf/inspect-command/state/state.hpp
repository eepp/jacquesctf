/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_STATE_HPP
#define _JACQUES_INSPECT_COMMAND_STATE_HPP

#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/core/noncopyable.hpp>

#include "message.hpp"
#include "data-stream-file-state.hpp"
#include "search-parser.hpp"
#include "data/packet-checkpoints-build-listener.hpp"
#include "data/trace.hpp"

namespace jacques {

class View;

/*
 * This state (application's model) guarantees the following:
 *
 * * There is at least one data stream file state.
 * * All data stream file state paths are unique.
 * * There is always an active data stream file state.
 * * All the contained data stream file states have a valid metadata
 *   object, because State::State() throws when there's any
 *   stream/parsing error.
 *
 * However, note that it is possible that any data stream file state,
 * including the active one, has no packets at all. This is only true
 * when the data stream file is empty, otherwise there's always at least
 * one available packet, but it could contain a decoding error.
 */
class State :
    boost::noncopyable
{
    friend class DataStreamFileState;
    friend class PacketState;

public:
    using Observer = std::function<void (const Message)>;

public:
    explicit State(const std::vector<boost::filesystem::path>& paths,
                   std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener);
    Index addObserver(const Observer& observer);
    void removeObserver(Index id);
    void gotoDataStreamFile(Index index);
    void gotoPreviousDataStreamFile();
    void gotoNextDataStreamFile();
    bool search(const SearchQuery& query);

    DataStreamFileState& activeDataStreamFileState() const
    {
        return *_activeDataStreamFileState;
    }

    Index activeDataStreamFileStateIndex() const
    {
        return _activeDataStreamFileStateIndex;
    }

    Size dataStreamFileStateCount() const
    {
        return _dataStreamFileStates.size();
    }

    const Metadata& metadata() const noexcept
    {
        return _activeDataStreamFileState->metadata();
    }

    void gotoPacket(const Index index)
    {
        _activeDataStreamFileState->gotoPacket(index);
    }

    void gotoPreviousPacket()
    {
        _activeDataStreamFileState->gotoPreviousPacket();
    }

    void gotoNextPacket()
    {
        _activeDataStreamFileState->gotoNextPacket();
    }

    void gotoPreviousEventRecord(const Size count = 1)
    {
        _activeDataStreamFileState->gotoPreviousEventRecord(count);
    }

    void gotoNextEventRecord(Size count = 1)
    {
        _activeDataStreamFileState->gotoNextEventRecord(count);
    }

    void gotoPreviousPacketRegion()
    {
        _activeDataStreamFileState->gotoPreviousPacketRegion();
    }

    void gotoNextPacketRegion()
    {
        _activeDataStreamFileState->gotoNextPacketRegion();
    }

    void gotoPacketContext()
    {
        _activeDataStreamFileState->gotoPacketContext();
    }

    void gotoLastPacketRegion()
    {
        _activeDataStreamFileState->gotoLastPacketRegion();
    }

    bool hasActivePacketState() const noexcept
    {
        return _activeDataStreamFileState->hasActivePacketState();
    }

    PacketState& activePacketState()
    {
        return _activeDataStreamFileState->activePacketState();
    }

    Index curOffsetInPacketBits() const noexcept
    {
        return _activeDataStreamFileState->curOffsetInPacketBits();
    }

    void gotoPacketRegionAtOffsetInPacketBits(const Index offsetInPacketBits)
    {
        _activeDataStreamFileState->gotoPacketRegionAtOffsetInPacketBits(offsetInPacketBits);
    }

    const EventRecord *currentEventRecord()
    {
        return _activeDataStreamFileState->currentEventRecord();
    }

    const PacketRegion *currentPacketRegion()
    {
        return _activeDataStreamFileState->currentPacketRegion();
    }

    const DataStreamFileState& dataStreamFileState(const Index index)
    {
        return *_dataStreamFileStates[index];
    }

    std::vector<std::unique_ptr<DataStreamFileState>>& dataStreamFileStates()
    {
        return _dataStreamFileStates;
    }

    const std::vector<std::unique_ptr<DataStreamFileState>>& dataStreamFileStates() const
    {
        return _dataStreamFileStates;
    }

private:
    void _notify(Message msg);

private:
    std::vector<Observer> _observers;
    std::vector<std::unique_ptr<DataStreamFileState>> _dataStreamFileStates;
    DataStreamFileState *_activeDataStreamFileState;
    Index _activeDataStreamFileStateIndex = 0;
    std::vector<std::unique_ptr<Trace>> _traces;
};

class StateObserverGuard
{
public:
    explicit StateObserverGuard(State& state, const State::Observer& observer);
    ~StateObserverGuard();

private:
    State * const _state;
    Index _observerId;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_STATE_HPP
