/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILE_STATE_HPP
#define _JACQUES_DATA_STREAM_FILE_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <yactfr/memory-mapped-file-view-factory.hpp>

#include "aliases.hpp"
#include "timestamp.hpp"
#include "data-stream-file.hpp"
#include "search-parser.hpp"
#include "packet.hpp"
#include "event-record.hpp"
#include "metadata.hpp"
#include "packet-state.hpp"

namespace jacques {

class State;

class DataStreamFileState
{
public:
    explicit DataStreamFileState(State& state,
                                 std::unique_ptr<DataStreamFile> dataStreamFile,
                                 std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener);
    void gotoOffsetBits(Index offsetBits);
    void gotoPacket(Index index);
    void gotoPreviousPacket();
    void gotoNextPacket();
    void gotoPreviousEventRecord(Size count = 1);
    void gotoNextEventRecord(Size count = 1);
    void gotoPreviousDataRegion();
    void gotoNextDataRegion();
    void gotoPacketContext();
    void gotoLastDataRegion();
    bool search(const SearchQuery& query);
    void analyzeAllPackets(PacketCheckpointsBuildListener& buildListener);

    DataStreamFile& dataStreamFile() noexcept
    {
        return *_dataStreamFile;
    }

    const DataStreamFile& dataStreamFile() const noexcept
    {
        return *_dataStreamFile;
    }

    void gotoOffsetBytes(Index offsetBytes)
    {
        return this->gotoOffsetBits(offsetBytes * 8);
    }

    bool hasActivePacketState() const noexcept
    {
        return _activePacketState != nullptr;
    }

    PacketState& activePacketState()
    {
        return this->_packetState(_activePacketStateIndex);
    }

    Index activePacketStateIndex() const noexcept
    {
        return _activePacketStateIndex;
    }

    Index curOffsetInPacketBits() const noexcept
    {
        if (!_activePacketState) {
            return 0;
        }

        return _activePacketState->curOffsetInPacketBits();
    }

    void gotoDataRegionAtOffsetInPacketBits(const Index offsetInPacketBits)
    {
        if (!_activePacketState) {
            return;
        }

        _activePacketState->gotoDataRegionAtOffsetInPacketBits(offsetInPacketBits);
    }

    const EventRecord *currentEventRecord()
    {
        if (!_activePacketState) {
            return nullptr;
        }

        return _activePacketState->currentEventRecord();
    }

    const DataRegion *currentDataRegion()
    {
        if (!_activePacketState) {
            return nullptr;
        }

        if (_activePacketState->packetIndexEntry().effectiveTotalSize() == 0) {
            return nullptr;
        }

        return &_activePacketState->currentDataRegion();
    }

    const Metadata& metadata() const noexcept
    {
        return _dataStreamFile->metadata();
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
    PacketState& _packetState(Index index);
    void _gotoPacket(Index index);
    bool _gotoNextEventRecordWithProperty(const std::function<bool (const EventRecord&)>& compareFunc,
                                          const boost::optional<Index>& initPacketIndex = boost::none,
                                          const boost::optional<Index>& initErIndex = boost::none);

private:
    State * const _state;
    PacketState *_activePacketState = nullptr;
    Index _activePacketStateIndex = 0;
    std::vector<std::unique_ptr<PacketState>> _packetStates;
    std::shared_ptr<PacketCheckpointsBuildListener> _packetCheckpointsBuildListener;
    std::unique_ptr<DataStreamFile> _dataStreamFile;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_STATE_HPP
