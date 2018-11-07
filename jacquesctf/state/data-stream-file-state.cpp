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
                                         DataStreamFile& dataStreamFile,
                                         std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener) :
    _state {&state},
    _packetCheckpointsBuildListener {std::move(packetCheckpointsBuildListener)},
    _dataStreamFile {&dataStreamFile}
{
}

void DataStreamFileState::gotoOffsetBits(const Index offsetBits)
{
    if (!_dataStreamFile->hasOffsetBits(offsetBits)) {
        return;
    }

    const auto& packetIndexEntry = _dataStreamFile->packetIndexEntryContainingOffsetBits(offsetBits);

    this->gotoPacket(packetIndexEntry.indexInDataStream());

    const auto offsetInPacketBits = offsetBits -
                                    packetIndexEntry.offsetInDataStreamBits();

    if (offsetInPacketBits > packetIndexEntry.effectiveTotalSize()) {
        // uh oh, that's outside the data we have for this invalid packet
        this->gotoLastPacketRegion();
        return;
    }

    const auto& region = _activePacketState->packet().packetRegionAtOffsetInPacketBits(offsetInPacketBits);

    _activePacketState->gotoPacketRegionAtOffsetInPacketBits(region);
}

PacketState& DataStreamFileState::_packetState(const Index index)
{
    if (_packetStates.size() < index + 1) {
        _packetStates.resize(index + 1);
    }

    if (!_packetStates[index]) {
        auto& packet = _dataStreamFile->packetAtIndex(index,
                                                      *_packetCheckpointsBuildListener);

        _packetStates[index] = std::make_unique<PacketState>(*_state, packet);
    }

    return *_packetStates[index];
}

void DataStreamFileState::_gotoPacket(const Index index)
{
    assert(index < _dataStreamFile->packetCount());
    _activePacketStateIndex = index;
    _activePacketState = &this->_packetState(index);
    _state->_notify(ActivePacketChangedMessage {});
}

void DataStreamFileState::gotoPacket(const Index index)
{
    assert(index < _dataStreamFile->packetCount());

    if (!_activePacketState) {
        // special case for the very first one, no notification required
        assert(index == 0);
        this->_gotoPacket(index);
        return;
    }

    if (_activePacketStateIndex == index) {
        return;
    }

    this->_gotoPacket(index);
}

void DataStreamFileState::gotoPreviousPacket()
{
    if (_dataStreamFile->packetCount() == 0) {
        return;
    }

    if (_activePacketStateIndex == 0) {
        return;
    }

    this->gotoPacket(_activePacketStateIndex - 1);
}

void DataStreamFileState::gotoNextPacket()
{
    if (_dataStreamFile->packetCount() == 0) {
        return;
    }

    if (_activePacketStateIndex == _dataStreamFile->packetCount() - 1) {
        return;
    }

    this->gotoPacket(_activePacketStateIndex + 1);
}

void DataStreamFileState::gotoPreviousEventRecord(Size count)
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoPreviousEventRecord(count);
}

void DataStreamFileState::gotoNextEventRecord(Size count)
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoNextEventRecord(count);
}

void DataStreamFileState::gotoPreviousPacketRegion()
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoPreviousPacketRegion();
}

void DataStreamFileState::gotoNextPacketRegion()
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoNextPacketRegion();
}

void DataStreamFileState::gotoPacketContext()
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoPacketContext();
}

void DataStreamFileState::gotoLastPacketRegion()
{
    if (!_activePacketState) {
        return;
    }

    _activePacketState->gotoLastPacketRegion();
}

bool DataStreamFileState::_gotoNextEventRecordWithProperty(const std::function<bool (const EventRecord&)>& compareFunc,
                                                           const boost::optional<Index>& initPacketIndex,
                                                           const boost::optional<Index>& initErIndex)
{
    if (!_activePacketState) {
        return false;
    }

    Index startPacketIndex = _activePacketStateIndex + 1;
    boost::optional<Index> startErIndex = 0;

    if (initPacketIndex) {
        startPacketIndex = *initPacketIndex;
    }

    if (initErIndex) {
        startErIndex = *initErIndex;
    }

    if (_activePacketState->packet().eventRecordCount() > 0 && !initPacketIndex && !initErIndex) {
        const auto currentEventRecord = _activePacketState->currentEventRecord();
        boost::optional<Index> erIndex;

        if (currentEventRecord) {
            if (currentEventRecord->indexInPacket() <
                    _activePacketState->packet().eventRecordCount() - 1) {
                // skip current event record
                startPacketIndex = _activePacketStateIndex;
                startErIndex = currentEventRecord->indexInPacket() + 1;
            }
        } else {
            const auto& firstEr = _activePacketState->packet().eventRecordAtIndexInPacket(0);

            if (_activePacketState->curOffsetInPacketBits() <
                    firstEr.segment().offsetInPacketBits()) {
                // search active packet from beginning
                startPacketIndex = _activePacketStateIndex;
            }
        }
    }

    for (auto packetIndex = startPacketIndex;
            packetIndex < _dataStreamFile->packetCount(); ++packetIndex) {
        auto& packetState = this->_packetState(packetIndex);
        auto& packet = packetState.packet();

        const auto iterStartErIndex = startErIndex ? *startErIndex : 0;

        startErIndex = boost::none;
        assert(iterStartErIndex < packet.eventRecordCount());

        for (Index erIndex = iterStartErIndex; erIndex < packet.eventRecordCount(); ++erIndex) {
            const auto& eventRecord = packet.eventRecordAtIndexInPacket(erIndex);

            if (compareFunc(eventRecord)) {
                const auto offsetInPacketBits = eventRecord.segment().offsetInPacketBits();

                this->gotoPacket(packetIndex);
                _activePacketState->gotoPacketRegionAtOffsetInPacketBits(offsetInPacketBits);
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
            reqIndex = static_cast<long long>(_activePacketStateIndex) +
                       sQuery->value();
        } else {
            // entry is natural (1-based)
            reqIndex = sQuery->value() - 1;
        }

        if (reqIndex < 0) {
            return false;
        }

        const auto index = static_cast<Index>(reqIndex);

        if (index >= _dataStreamFile->packetCount()) {
            return false;
        }

        this->gotoPacket(index);
        return true;
    } else if (const auto sQuery = dynamic_cast<const PacketSeqNumSearchQuery *>(&query)) {
        if (!_activePacketState) {
            return false;
        }

        long long reqSeqNum;

        if (sQuery->isDiff()) {
            const auto& indexEntry = _activePacketState->packetIndexEntry();

            if (!indexEntry.seqNum()) {
                return false;
            }

            reqSeqNum = static_cast<long long>(*indexEntry.seqNum()) +
                        sQuery->value();
        } else {
            reqSeqNum = sQuery->value();
        }

        if (reqSeqNum < 0) {
            return false;
        }

        const auto indexEntry = _dataStreamFile->packetIndexEntryWithSeqNum(static_cast<Index>(reqSeqNum));

        if (!indexEntry) {
            return false;
        }

        this->gotoPacket(indexEntry->indexInDataStream());
        return true;
    } else if (const auto sQuery = dynamic_cast<const EventRecordIndexSearchQuery *>(&query)) {
        if (!_activePacketState) {
            return false;
        }

        long long reqIndex;

        if (sQuery->isDiff()) {
            const auto curEventRecord = _activePacketState->currentEventRecord();

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

        if (index >= _activePacketState->packet().eventRecordCount()) {
            return false;
        }

        const auto& eventRecord = _activePacketState->packet().eventRecordAtIndexInPacket(index);

        this->gotoPacketRegionAtOffsetInPacketBits(eventRecord.segment().offsetInPacketBits());
        return true;
    } else if (const auto sQuery = dynamic_cast<const OffsetSearchQuery *>(&query)) {
        long long reqOffsetBits;

        if (sQuery->target() == OffsetSearchQuery::Target::PACKET &&
                !_activePacketState) {
            return false;
        }

        if (sQuery->isDiff()) {
            switch (sQuery->target()) {
            case OffsetSearchQuery::Target::PACKET:
                reqOffsetBits = static_cast<long long>(_activePacketState->curOffsetInPacketBits()) +
                                sQuery->value();
                break;

            case OffsetSearchQuery::Target::DATA_STREAM_FILE:
            {
                const auto curPacketOffsetBitsInDataStream = _activePacketState->packetIndexEntry().offsetInDataStreamBits();

                reqOffsetBits = static_cast<long long>(curPacketOffsetBitsInDataStream +
                                                       _activePacketState->curOffsetInPacketBits()) +
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
            if (offsetInPacketBits >= _activePacketState->packetIndexEntry().effectiveTotalSize()) {
                return false;
            }

            const auto& region = _activePacketState->packet().packetRegionAtOffsetInPacketBits(offsetInPacketBits);

            _activePacketState->gotoPacketRegionAtOffsetInPacketBits(region);
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
    } else if (const auto sQuery = dynamic_cast<const TimestampSearchQuery *>(&query)) {
        if (!_activePacketState) {
            return false;
        }

        auto reqValue = sQuery->value();

        if (sQuery->isDiff()) {
            const auto curEventRecord = _activePacketState->currentEventRecord();

            if (!curEventRecord || !curEventRecord->firstTimestamp()) {
                return false;
            }

            switch (sQuery->unit()) {
            case TimestampSearchQuery::Unit::NS:
                reqValue += curEventRecord->firstTimestamp()->nsFromOrigin();
                break;

            case TimestampSearchQuery::Unit::CYCLE:
                reqValue += static_cast<long long>(curEventRecord->firstTimestamp()->cycles());
                break;
            }
        }

        const PacketIndexEntry *indexEntry = nullptr;

        switch (sQuery->unit()) {
        case TimestampSearchQuery::Unit::NS:
            indexEntry = _dataStreamFile->packetIndexEntryContainingNsFromOrigin(reqValue);
            break;

        case TimestampSearchQuery::Unit::CYCLE:
            indexEntry = _dataStreamFile->packetIndexEntryContainingCycles(static_cast<unsigned long long>(reqValue));
            break;
        }

        if (!indexEntry) {
            return false;
        }

        auto& packet = _dataStreamFile->packetAtIndex(indexEntry->indexInDataStream(),
                                                      *_packetCheckpointsBuildListener);
        const EventRecord *eventRecord = nullptr;

        switch (sQuery->unit()) {
        case TimestampSearchQuery::Unit::NS:
            eventRecord = packet.eventRecordAtOrAfterNsFromOrigin(reqValue);
            break;

        case TimestampSearchQuery::Unit::CYCLE:
            eventRecord = packet.eventRecordAtOrAfterCycles(static_cast<unsigned long long>(reqValue));
            break;
        }

        if (!eventRecord) {
            return false;
        }

        // `eventRecord` can become invalid through `this->gotoPacket()`
        const auto offsetInPacketBits = eventRecord->segment().offsetInPacketBits();

        if (_activePacketState->packet().indexEntry() != *indexEntry) {
            // change packet
            this->gotoPacket(indexEntry->indexInDataStream());
        }

        _activePacketState->gotoPacketRegionAtOffsetInPacketBits(offsetInPacketBits);
        return true;
    }

    return false;
}

void DataStreamFileState::analyzeAllPackets(PacketCheckpointsBuildListener& buildListener)
{
    for (auto& pktIndexEntry : _dataStreamFile->packetIndexEntries()) {
        if (pktIndexEntry.eventRecordCount()) {
            continue;
        }

        // this creates checkpoints and shows progress
        _dataStreamFile->packetAtIndex(pktIndexEntry.indexInDataStream(),
                                       buildListener);
    }
}

} // namespace jacques
