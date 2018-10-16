/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
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
    if (!_dataStreamFile.hasOffsetBits(offsetBits)) {
        return;
    }

    const auto& packetIndexEntry = _dataStreamFile.packetIndexEntryContainingOffsetBits(offsetBits);

    this->gotoPacket(packetIndexEntry.indexInDataStream());

    DataRegions dataRegions;
    const auto offsetInPacketBits = offsetBits -
                                    packetIndexEntry.offsetInDataStreamBits();

    _activePacket->appendDataRegions(dataRegions, offsetInPacketBits,
                                     offsetInPacketBits + 1);
    assert(dataRegions.size() == 1);
    _activePacket->curOffsetInPacketBits(dataRegions.front()->segment().offsetInPacketBits());
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
    if (_dataStreamFile.packetCount() == 0) {
        return;
    }

    if (_activePacketIndex == 0) {
        return;
    }

    this->gotoPacket(_activePacketIndex - 1);
}

void DataStreamFileState::gotoNextPacket()
{
    if (_dataStreamFile.packetCount() == 0) {
        return;
    }

    if (_activePacketIndex == _dataStreamFile.packetCount() - 1) {
        return;
    }

    this->gotoPacket(_activePacketIndex + 1);
}

void DataStreamFileState::gotoPreviousEventRecord(Size count)
{
    if (!_activePacket) {
        return;
    }

    if (_activePacket->eventRecordCount() == 0) {
        return;
    }

    const auto curEventRecord = this->currentEventRecord();

    if (!curEventRecord) {
        if (_activePacket->curOffsetInPacketBits() >=
                _activePacket->indexEntry().effectiveContentSize().bits()) {
            auto& lastEr = _activePacket->eventRecordAtIndexInPacket(_state->activePacket().eventRecordCount() - 1);

            _activePacket->curOffsetInPacketBits(lastEr.segment().offsetInPacketBits());
        }

        return;
    }

    if (curEventRecord->indexInPacket() == 0) {
        return;
    }

    count = std::min(curEventRecord->indexInPacket(), count);
    const auto& prevEventRecord = _activePacket->eventRecordAtIndexInPacket(curEventRecord->indexInPacket() - count);
    _activePacket->curOffsetInPacketBits(prevEventRecord.segment().offsetInPacketBits());
}

void DataStreamFileState::gotoNextEventRecord(Size count)
{
    if (!_activePacket) {
        return;
    }

    if (_activePacket->eventRecordCount() == 0) {
        return;
    }

    const auto curEventRecord = this->currentEventRecord();
    Index newIndex = 0;

    if (curEventRecord) {
        count = std::min(_activePacket->eventRecordCount() -
                         curEventRecord->indexInPacket(), count);
        newIndex = curEventRecord->indexInPacket() + count;
    }

    if (newIndex >= _activePacket->eventRecordCount()) {
        return;
    }

    const auto& nextEventRecord = _activePacket->eventRecordAtIndexInPacket(newIndex);
    _activePacket->curOffsetInPacketBits(nextEventRecord.segment().offsetInPacketBits());
}

void DataStreamFileState::gotoPreviousDataRegion()
{
    if (!_activePacket) {
        return;
    }

    if (_activePacket->curOffsetInPacketBits() == 0) {
        return;
    }

    const auto& prevDataRegion = _activePacket->dataRegionAtOffsetInPacketBits(_activePacket->curOffsetInPacketBits() - 1);

    _activePacket->curOffsetInPacketBits(prevDataRegion.segment().offsetInPacketBits());
}

void DataStreamFileState::gotoNextDataRegion()
{
    const auto currentDataRegion = this->currentDataRegion();

    if (!currentDataRegion) {
        return;
    }

    if (currentDataRegion->segment().endOffsetInPacketBits() ==
            _activePacket->indexEntry().effectiveTotalSize().bits()) {
        return;
    }

    _activePacket->curOffsetInPacketBits(currentDataRegion->segment().endOffsetInPacketBits());
}

void DataStreamFileState::gotoPacketContext()
{
    if (!_activePacket) {
        return;
    }

    const auto& offset = _activePacket->indexEntry().packetContextOffsetInPacketBits();

    if (!offset) {
        return;
    }

    _activePacket->curOffsetInPacketBits(*offset);
}

void DataStreamFileState::gotoLastDataRegion()
{
    if (!_activePacket) {
        return;
    }

    _activePacket->curOffsetInPacketBits(_activePacket->lastDataRegion().segment().offsetInPacketBits());
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
        packet = std::make_shared<Packet>(*this, packetIndexEntry, _seq,
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
