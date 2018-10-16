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
#include "lru-cache.hpp"
#include "packet-checkpoints-build-listener.hpp"

namespace jacques {

class State;

class DataStreamFileState
{
public:
    explicit DataStreamFileState(State& state,
                                 const boost::filesystem::path& path,
                                 std::shared_ptr<const Metadata> metadata,
                                 std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener);
    ~DataStreamFileState();
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
        return _dataStreamFile;
    }

    const DataStreamFile& dataStreamFile() const noexcept
    {
        return _dataStreamFile;
    }

    void gotoOffsetBytes(Index offsetBytes)
    {
        return this->gotoOffsetBits(offsetBytes * 8);
    }

    bool hasActivePacket() const noexcept
    {
        return _activePacket != nullptr;
    }

    Packet& activePacket()
    {
        if (!_activePacket) {
            _activePacket = this->_packet(_activePacketIndex,
                                          *_packetCheckpointsBuildListener);
        }

        return *_activePacket;
    }

    Index activePacketIndex() const
    {
        return _activePacketIndex;
    }

    Index curOffsetInPacketBits() const noexcept
    {
        if (!_activePacket) {
            return 0;
        }

        return _activePacket->curOffsetInPacketBits();
    }

    void curOffsetInPacketBits(const Index offsetInPacketBits)
    {
        if (!_activePacket) {
            return;
        }

        if (!_activePacket->hasData()) {
            return;
        }

        _activePacket->curOffsetInPacketBits(offsetInPacketBits);
    }

    const EventRecord *currentEventRecord()
    {
        if (!_activePacket) {
            return nullptr;
        }

        return _activePacket->currentEventRecord();
    }

    const DataRegion *currentDataRegion()
    {
        if (!_activePacket) {
            return nullptr;
        }

        if (_activePacket->indexEntry().effectiveTotalSize() == 0){
            return nullptr;
        }

        return _activePacket->currentDataRegion();
    }

    const Metadata& metadata() const noexcept
    {
        return *_metadata;
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
    Packet::SP _packet(Index index, PacketCheckpointsBuildListener& buildListener);
    void _gotoPacket(Index index);

private:
    State *_state;
    Packet::SP _activePacket;
    Index _activePacketIndex;
    std::shared_ptr<const Metadata> _metadata;
    std::shared_ptr<PacketCheckpointsBuildListener> _packetCheckpointsBuildListener;
    std::shared_ptr<yactfr::MemoryMappedFileViewFactory> _factory;
    yactfr::PacketSequence _seq;
    DataStreamFile _dataStreamFile;
    LruCache<Index, Packet::SP> _packetCache;
    int _fd;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_STATE_HPP
