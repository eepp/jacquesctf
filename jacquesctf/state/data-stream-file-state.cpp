/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data-stream-file-state.hpp"
#include "active-packet-changed-message.hpp"
#include "state.hpp"
#include "io-error.hpp"

namespace jacques {

DataStreamFileState::DataStreamFileState(State& state,
                                         const boost::filesystem::path& path,
                                         std::shared_ptr<const Metadata> metadata,
                                         std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener) :
    _state {&state},
    _activePacketIndex {0},
    _metadata {metadata},
    _packetCheckpointsBuildListener {packetCheckpointsBuildListener},
    _factory {
        std::make_shared<yactfr::MemoryMappedFileViewFactory>(path.string(),
                                                              8 << 20,
                                                              yactfr::MemoryMappedFileViewFactory::AccessPattern::SEQUENTIAL)
    },
    _seq {_metadata->traceType(), _factory},
    _dataStreamFile {path, *metadata, _seq, *_factory},
    _packetCache {16}
{
    _factory->expectedAccessPattern(yactfr::MemoryMappedFileViewFactory::AccessPattern::SEQUENTIAL);
    _fd = open(path.string().c_str(), O_RDONLY);

    if (_fd < 0) {
        throw IOError {path, "Cannot open file."};
    }
}

DataStreamFileState::~DataStreamFileState()
{
    if (_fd >= 0) {
        (void) close(_fd);
    }
}

void DataStreamFileState::gotoOffsetBits(const Index offsetBits)
{
}

void DataStreamFileState::gotoOffsetBytes(const Index offsetBytes)
{
}

void DataStreamFileState::_gotoPacket(const Index index)
{
    _activePacketIndex = index;
    _activePacket = this->_packet(index, *_packetCheckpointsBuildListener);
    _state->_notify(ActivePacketChangedMessage {*this, index});
}

void DataStreamFileState::gotoPacket(const Index index)
{
    assert(index < _dataStreamFile.packetCount());

    if (!_activePacket) {
        // special case for the very first one, no notification required
        assert(index == 0);
        this->_gotoPacket(index);
        return;
    }

    if (_activePacketIndex == index) {
        return;
    }

    this->_gotoPacket(index);
}

void DataStreamFileState::gotoPreviousPacket()
{
    if (_activePacketIndex == 0) {
        return;
    }

    this->gotoPacket(_activePacketIndex - 1);
}

void DataStreamFileState::gotoNextPacket()
{
    if (_activePacketIndex == _dataStreamFile.packetCount() - 1) {
        return;
    }

    this->gotoPacket(_activePacketIndex + 1);
}

bool DataStreamFileState::search(const SearchQuery& query)
{
    return false;
}

Packet::SP DataStreamFileState::_packet(const Index index,
                                        PacketCheckpointsBuildListener& buildListener)
{
    assert(index < _dataStreamFile.packetCount());

    Packet::SP packet;

    auto packetPtr = _packetCache.get(index);

    if (packetPtr) {
        packet = *packetPtr;
    } else {
        auto& packetIndexEntry = _dataStreamFile.packetIndexEntry(index);
        auto mmapFile = std::make_unique<MemoryMappedFile>(_dataStreamFile.path(),
                                                           _fd);

        buildListener.startBuild(packetIndexEntry);
        packet = std::make_shared<Packet>(packetIndexEntry, _seq,
                                          *_metadata,
                                          _factory->createDataSource(),
                                          std::move(mmapFile),
                                          buildListener);
        buildListener.endBuild();

        if (packet->error()) {
            packetIndexEntry.isInvalid(true);
        }

        packetIndexEntry.eventRecordCount(packet->eventRecordCount());
        _packetCache.insert(index, packet);
    }

    return packet;
}

void DataStreamFileState::analyzeAllPackets(PacketCheckpointsBuildListener& buildListener)
{
    for (auto& pktIndexEntry : _dataStreamFile.packetIndexEntries()) {
        if (pktIndexEntry.eventRecordCount()) {
            continue;
        }

        // this creates checkpoints and shows progress
        this->_packet(pktIndexEntry.indexInDataStream(), buildListener);
    }
}

} // namespace jacques
