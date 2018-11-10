/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data-stream-file.hpp"
#include "io-error.hpp"

namespace jacques {

DataStreamFile::DataStreamFile(const boost::filesystem::path& path,
                               const Metadata& metadata) :
    _path {path},
    _metadata {&metadata},
    _factory {
        std::make_shared<yactfr::MemoryMappedFileViewFactory>(path.string(),
                                                              8 << 20,
                                                              yactfr::MemoryMappedFileViewFactory::AccessPattern::SEQUENTIAL)
    },
    _seq {_metadata->traceType(), _factory}
{
    _fileSize = DataSize::fromBytes(boost::filesystem::file_size(path));
    _fd = open(path.string().c_str(), O_RDONLY);

    if (_fd < 0) {
        throw IOError {path, "Cannot open file."};
    }
}

DataStreamFile::~DataStreamFile()
{
    if (_fd >= 0) {
        (void) close(_fd);
    }
}

void DataStreamFile::buildIndex(const BuildIndexProgressFunc& progressFunc,
                                const Size step)
{
    if (_isIndexBuilt) {
        return;
    }

    if (_fileSize == 0) {
        _isIndexBuilt = true;
        return;
    }

    const auto oldExpectedAccessPattern = _factory->expectedAccessPattern();

    _factory->expectedAccessPattern(yactfr::MemoryMappedFileViewFactory::AccessPattern::RANDOM);
    this->_buildIndex(progressFunc, step);
    _factory->expectedAccessPattern(oldExpectedAccessPattern);
    _isIndexBuilt = true;
    _packets.resize(_index.size());
}

void DataStreamFile::_addPacketIndexEntry(const Index offsetInDataStreamBytes,
                                          const _IndexBuildingState& state,
                                          bool isInvalid)
{
    auto expectedTotalSize = state.expectedTotalSize;
    auto expectedContentSize = state.expectedContentSize;

    if (!expectedTotalSize && !isInvalid) {
        expectedTotalSize = _fileSize;
    }

    if (!expectedContentSize && !isInvalid) {
        expectedContentSize = expectedTotalSize;
    }

    const auto availSize = DataSize::fromBytes(_fileSize.bytes() -
                                               offsetInDataStreamBytes);
    auto effectiveTotalSize = expectedTotalSize ? *expectedTotalSize : 0;

    if (effectiveTotalSize > availSize) {
        // not enough data
        isInvalid = true;
        effectiveTotalSize = availSize;
        _isComplete = false;
    }

    auto effectiveContentSize = expectedContentSize ? *expectedContentSize : 0;

    if (effectiveContentSize > effectiveTotalSize) {
        isInvalid = true;
        effectiveContentSize = effectiveTotalSize;
        _isComplete = false;
    }

    _index.push_back(PacketIndexEntry {
        _index.size(), offsetInDataStreamBytes,
        state.packetContextOffsetInPacketBits,
        state.preambleSize,
        *expectedTotalSize, *expectedContentSize,
        effectiveTotalSize, effectiveContentSize,
        state.dst, state.dataStreamId, state.tsBegin, state.tsEnd, state.seqNum,
        state.discardedEventRecordCounter, isInvalid,
    });
}

void DataStreamFile::_IndexBuildingState::reset()
{
    inPacketContextScope = false;
    preambleSize = boost::none;
    packetContextOffsetInPacketBits = boost::none;
    expectedTotalSize = boost::none;
    expectedContentSize = boost::none;
    tsBegin = boost::none;
    tsEnd = boost::none;
    seqNum = boost::none;
    dataStreamId = boost::none;
    discardedEventRecordCounter = boost::none;
    dst = nullptr;
}

void DataStreamFile::_buildIndex(const BuildIndexProgressFunc& progressFunc,
                                 const Size step)
{
    auto it = std::begin(_seq);
    const auto endIt = std::end(_seq);
    Index offsetBytes = 0;
    _IndexBuildingState state;
    bool packetStarted = false;

    try {
        while (it != endIt) {
            switch (it->kind()) {
            case yactfr::Element::Kind::PACKET_BEGINNING:
                offsetBytes = it.offset() / 8;
                packetStarted = true;
                break;

            case yactfr::Element::Kind::SCOPE_BEGINNING:
            {
                auto& elem = static_cast<const yactfr::ScopeBeginningElement&>(*it);

                if (elem.scope() == yactfr::Scope::PACKET_CONTEXT) {
                    state.inPacketContextScope = true;
                    state.packetContextOffsetInPacketBits = it.offset() -
                                                            (offsetBytes * 8);
                }

                break;
            }

            case yactfr::Element::Kind::PACKET_CONTENT_END:
            case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
            {
                state.preambleSize = it.offset() - offsetBytes * 8;
                state.inPacketContextScope = false;
                this->_addPacketIndexEntry(offsetBytes, state, false);

                if (_index.size() % step == 0) {
                    progressFunc(_index.back());
                }

                state.reset();

                /*
                 * If the effective total size is not the expected total
                 * size, then it covers the whole file anyway, so
                 * `nextOffsetBytes` below will be equal to
                 * _fileSize.bytes().
                 */
                const auto nextOffsetBytes = offsetBytes +
                                             _index.back().effectiveTotalSize().bytes();

                if (nextOffsetBytes >= _fileSize.bytes()) {
                    it = endIt;
                } else {
                    it.seekPacket(nextOffsetBytes);
                }

                continue;
            }

            case yactfr::Element::Kind::EXPECTED_PACKET_TOTAL_SIZE:
            {
                auto& elem = static_cast<const yactfr::ExpectedPacketTotalSizeElement&>(*it);

                state.expectedTotalSize = elem.expectedSize();
                break;
            }

            case yactfr::Element::Kind::EXPECTED_PACKET_CONTENT_SIZE:
            {
                auto& elem = static_cast<const yactfr::ExpectedPacketContentSizeElement&>(*it);

                state.expectedContentSize = elem.expectedSize();
                break;
            }

            case yactfr::Element::Kind::CLOCK_VALUE:
            {
                if (!state.inPacketContextScope || state.tsBegin ||
                        !_metadata->isCorrelatable()) {
                    break;
                }

                auto& elem = static_cast<const yactfr::ClockValueElement&>(*it);

                state.tsBegin = Timestamp {elem};
                break;
            }

            case yactfr::Element::Kind::PACKET_END_CLOCK_VALUE:
            {
                if (!state.inPacketContextScope || state.tsEnd ||
                        !_metadata->isCorrelatable()) {
                    break;
                }

                auto& elem = static_cast<const yactfr::PacketEndClockValueElement&>(*it);

                state.tsEnd = Timestamp {elem};
                break;
            }

            case yactfr::Element::Kind::DATA_STREAM_ID:
            {
                auto& elem = static_cast<const yactfr::DataStreamIdElement&>(*it);

                state.dataStreamId = elem.id();
                break;
            }

            case yactfr::Element::Kind::PACKET_ORIGIN_INDEX:
            {
                auto& elem = static_cast<const yactfr::PacketOriginIndexElement&>(*it);

                state.seqNum = elem.index();
                break;
            }

            case yactfr::Element::Kind::UNSIGNED_INT:
            {
                if (state.inPacketContextScope) {
                    auto& elem = static_cast<const yactfr::UnsignedIntElement&>(*it);

                    if (elem.displayName() &&
                            *elem.displayName() == "events_discarded") {
                        state.discardedEventRecordCounter = elem.value();
                    }
                }

                break;
            }

            case yactfr::Element::Kind::DATA_STREAM_TYPE:
            {
                auto& elem = static_cast<const yactfr::DataStreamTypeElement&>(*it);

                state.dst = &elem.dataStreamType();
                break;
            }

            default:
                break;
            }

            ++it;
        }
    } catch (const yactfr::DecodingError& ex) {
        if (packetStarted) {
            /*
             * Error while reading the packet before creating an index
             * entry: create an invalid entry so that we know about
             * this.
             */
            this->_addPacketIndexEntry(offsetBytes, state, true);
        }
    }
}

bool DataStreamFile::hasOffsetBits(const Index offsetBits)
{
    assert(_isIndexBuilt);

    if (_fileSize == 0) {
        return false;
    }

    assert(!_index.empty());

    return offsetBits < _index.back().endOffsetInDataStreamBits();
}

const PacketIndexEntry& DataStreamFile::packetIndexEntryContainingOffsetBits(const Index offsetBits)
{
    assert(this->hasOffsetBits(offsetBits));

    auto it = std::upper_bound(std::begin(_index), std::end(_index),
                               offsetBits, [](const auto offsetBits,
                                              const auto& entry) {
        return offsetBits <
               entry.offsetInDataStreamBits() + entry.effectiveTotalSize();
    });

    assert(it != std::end(_index));
    assert(offsetBits >= it->offsetInDataStreamBits() &&
           offsetBits < it->offsetInDataStreamBits() +
                        it->effectiveTotalSize());


    return *it;
}

const PacketIndexEntry *DataStreamFile::packetIndexEntryContainingNsFromOrigin(const long long nsFromOrigin)
{
    const auto tsLtCompFunc = [](const Timestamp& ts,
                                 const long long nsFromOrigin) -> bool {
        return ts.nsFromOrigin() < nsFromOrigin;
    };
    const auto valueInTsFunc = [](const long long nsFromOrigin,
                                  const PacketIndexEntry& entry) -> bool {
        return nsFromOrigin >= entry.beginningTimestamp()->nsFromOrigin() &&
               nsFromOrigin < entry.endTimestamp()->nsFromOrigin();
    };

    return this->_packetIndexEntryContainingValue(tsLtCompFunc, valueInTsFunc,
                                                  nsFromOrigin);
}

const PacketIndexEntry *DataStreamFile::packetIndexEntryContainingCycles(const unsigned long long cycles)
{
    const auto tsLtCompFunc = [](const Timestamp& ts,
                                 const unsigned long long cycles) -> bool {
        return ts.cycles() < cycles;
    };
    const auto valueInTsFunc = [](const unsigned long long cycles,
                                  const PacketIndexEntry& entry) -> bool {
        return cycles >= entry.beginningTimestamp()->cycles() &&
               cycles < entry.endTimestamp()->cycles();
    };

    return this->_packetIndexEntryContainingValue(tsLtCompFunc, valueInTsFunc,
                                                  cycles);
}

const PacketIndexEntry *DataStreamFile::packetIndexEntryWithSeqNum(const Index seqNum)
{
    assert(_isIndexBuilt);

    if (_index.empty()) {
        return nullptr;
    }

    auto it = std::upper_bound(std::begin(_index), std::end(_index),
                               seqNum, [](const auto seqNum,
                                          const auto& entry) {
        if (entry.seqNum()) {
            return seqNum < *entry.seqNum();
        }

        return true;
    });

    --it;

    if (!it->seqNum()) {
        return nullptr;
    }

    if (*it->seqNum() != seqNum) {
        return nullptr;
    }

    return &(*it);
}

Packet& DataStreamFile::packetAtIndex(const Index index,
                                      PacketCheckpointsBuildListener& buildListener)
{
    assert(_isIndexBuilt);
    assert(index < _index.size());

    if (!_packets[index]) {
        auto& packetIndexEntry = _index[index];
        auto mmapFile = std::make_unique<MemoryMappedFile>(_path, _fd);

        buildListener.startBuild(packetIndexEntry);

        auto packet = std::make_unique<Packet>(packetIndexEntry, _seq,
                                               *_metadata,
                                               _factory->createDataSource(),
                                               std::move(mmapFile),
                                               buildListener);

        buildListener.endBuild();

        if (packet->error()) {
            packetIndexEntry.isInvalid(true);
        }

        packetIndexEntry.eventRecordCount(packet->eventRecordCount());
        _packets[index] = std::move(packet);
    }

    return *_packets[index];
}

} // namespace jacques
