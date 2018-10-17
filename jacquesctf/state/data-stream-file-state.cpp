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
#include "search-parser.hpp"
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

    const auto offsetInPacketBits = offsetBits -
                                    packetIndexEntry.offsetInDataStreamBits();

    if (offsetInPacketBits > packetIndexEntry.effectiveTotalSize()) {
        // uh oh, that's outside the data we have for this invalid packet
        this->gotoLastDataRegion();
        return;
    }

    const auto& region = _activePacket->dataRegionAtOffsetInPacketBits(offsetInPacketBits);

    _activePacket->curOffsetInPacketBits(region.segment().offsetInPacketBits());
}

void DataStreamFileState::_gotoPacket(const Index index)
{
    _activePacketIndex = index;
    _activePacket = this->_packet(index, *_packetCheckpointsBuildListener);
    _state->_notify(ActivePacketChangedMessage {});
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

    const auto currentDataRegion = this->currentDataRegion();

    assert(currentDataRegion);

    if (currentDataRegion->previousDataRegionOffsetInPacketBits()) {
        _activePacket->curOffsetInPacketBits(*currentDataRegion->previousDataRegionOffsetInPacketBits());
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

bool DataStreamFileState::_gotoNextEventRecordWithProperty(const std::function<bool (const EventRecord&)>& compareFunc,
                                                           const boost::optional<Index>& initPacketIndex,
                                                           const boost::optional<Index>& initErIndex)
{
    if (!_activePacket) {
        return false;
    }

    Index startPacketIndex = _activePacketIndex + 1;
    boost::optional<Index> startErIndex = 0;

    if (initPacketIndex) {
        startPacketIndex = *initPacketIndex;
    }

    if (initErIndex) {
        startErIndex = *initErIndex;
    }

    if (_activePacket->eventRecordCount() > 0 && !initPacketIndex && !initErIndex) {
        const auto currentEventRecord = _activePacket->currentEventRecord();
        boost::optional<Index> erIndex;

        if (currentEventRecord) {
            if (currentEventRecord->indexInPacket() <
                    _activePacket->eventRecordCount() - 1) {
                // skip current event record
                startPacketIndex = _activePacketIndex;
                startErIndex = currentEventRecord->indexInPacket() + 1;
            }
        } else {
            const auto& firstEr = _activePacket->eventRecordAtIndexInPacket(0);

            if (_activePacket->curOffsetInPacketBits() <
                    firstEr.segment().offsetInPacketBits()) {
                // search active packet from beginning
                startPacketIndex = _activePacketIndex;
            }
        }
    }

    for (auto packetIndex = startPacketIndex;
            packetIndex < _dataStreamFile.packetCount(); ++packetIndex) {
        auto packet = this->_packet(packetIndex,
                                    *_packetCheckpointsBuildListener);

        assert(packet);

        const auto iterStartErIndex = startErIndex ? *startErIndex : 0;

        startErIndex = boost::none;
        assert(iterStartErIndex < packet->eventRecordCount());

        for (Index erIndex = iterStartErIndex; erIndex < packet->eventRecordCount(); ++erIndex) {
            const auto& eventRecord = packet->eventRecordAtIndexInPacket(erIndex);

            if (compareFunc(eventRecord)) {
                const auto offsetInPacketBits = eventRecord.segment().offsetInPacketBits();

                this->gotoPacket(packetIndex);
                _activePacket->curOffsetInPacketBits(offsetInPacketBits);
                return true;
            }
        }
    }

    return false;
}

bool DataStreamFileState::search(const SearchQuery& query)
{
    if (const auto sQuery = dynamic_cast<const PacketIndexSearchQuery *>(&query)) {
        long long reqIndex;

        if (sQuery->isDiff()) {
            reqIndex = static_cast<long long>(_activePacketIndex) +
                       sQuery->value();
        } else {
            // entry is natural (1-based)
            reqIndex = sQuery->value() - 1;
        }

        if (reqIndex < 0) {
            return false;
        }

        const auto index = static_cast<Index>(reqIndex);

        if (index >= _dataStreamFile.packetCount()) {
            return false;
        }

        this->gotoPacket(index);
        return true;
    } else if (const auto sQuery = dynamic_cast<const PacketSeqNumSearchQuery *>(&query)) {
        if (!_activePacket) {
            return false;
        }

        long long reqSeqNum;

        if (sQuery->isDiff()) {
            if (!_activePacket->indexEntry().seqNum()) {
                return false;
            }

            reqSeqNum = static_cast<long long>(*_activePacket->indexEntry().seqNum()) +
                        sQuery->value();
        } else {
            reqSeqNum = sQuery->value();
        }

        if (reqSeqNum < 0) {
            return false;
        }

        const auto indexEntry = _dataStreamFile.packetIndexEntryWithSeqNum(static_cast<Index>(reqSeqNum));

        if (!indexEntry) {
            return false;
        }

        this->gotoPacket(indexEntry->indexInDataStream());
        return true;
    } else if (const auto sQuery = dynamic_cast<const EventRecordIndexSearchQuery *>(&query)) {
        if (!_activePacket) {
            return false;
        }

        long long reqIndex;

        if (sQuery->isDiff()) {
            const auto curEventRecord = _activePacket->currentEventRecord();

            if (!curEventRecord) {
                return false;
            }

            reqIndex = static_cast<long long>(curEventRecord->indexInPacket()) +
                       sQuery->value();
        } else {
            // entry is natural (1-based)
            reqIndex = sQuery->value() - 1;
        }

        if (reqIndex < 0) {
            return false;
        }

        const auto index = static_cast<Index>(reqIndex);

        if (index >= _activePacket->eventRecordCount()) {
            return false;
        }

        const auto& eventRecord = _activePacket->eventRecordAtIndexInPacket(index);

        _activePacket->curOffsetInPacketBits(eventRecord.segment().offsetInPacketBits());
        return true;
    } else if (const auto sQuery = dynamic_cast<const OffsetSearchQuery *>(&query)) {
        long long reqOffsetBits;

        if (sQuery->target() == OffsetSearchQuery::Target::PACKET &&
                !_activePacket) {
            return false;
        }

        if (sQuery->isDiff()) {
            switch (sQuery->target()) {
            case OffsetSearchQuery::Target::PACKET:
                reqOffsetBits = static_cast<long long>(_activePacket->curOffsetInPacketBits()) +
                                sQuery->value();
                break;

            case OffsetSearchQuery::Target::DATA_STREAM_FILE:
            {
                const auto curPacketOffsetBitsInDataStream = _activePacket->indexEntry().offsetInDataStreamBits();

                reqOffsetBits = static_cast<long long>(curPacketOffsetBitsInDataStream +
                                                       _activePacket->curOffsetInPacketBits()) +
                                sQuery->value();
                break;
            }
            }
        } else {
            reqOffsetBits = sQuery->value();
        }

        if (reqOffsetBits < 0) {
            return false;
        }

        const auto offsetInPacketBits = static_cast<Index>(reqOffsetBits);

        switch (sQuery->target()) {
        case OffsetSearchQuery::Target::PACKET:
        {
            if (offsetInPacketBits >= _activePacket->indexEntry().effectiveTotalSize()) {
                return false;
            }

            const auto& region = _activePacket->dataRegionAtOffsetInPacketBits(offsetInPacketBits);

            _activePacket->curOffsetInPacketBits(region.segment().offsetInPacketBits());
            break;
        }

        case OffsetSearchQuery::Target::DATA_STREAM_FILE:
            this->gotoOffsetBits(offsetInPacketBits);
            break;
        }

        return true;
    } else if (const auto sQuery = dynamic_cast<const EventRecordTypeIdSearchQuery *>(&query)) {
        if (sQuery->value() < 0) {
            return false;
        }

        const auto compareFunc = [sQuery](const EventRecord& eventRecord) {
            return eventRecord.type().id() == static_cast<Index>(sQuery->value());
        };

        return this->_gotoNextEventRecordWithProperty(compareFunc);
    } else if (const auto sQuery = dynamic_cast<const EventRecordTypeNameSearchQuery *>(&query)) {
        const auto compareFunc = [sQuery](const EventRecord& eventRecord) {
            if (!eventRecord.type().name()) {
                return false;
            }

            return sQuery->matches(*eventRecord.type().name());
        };

        return this->_gotoNextEventRecordWithProperty(compareFunc);
    }

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
