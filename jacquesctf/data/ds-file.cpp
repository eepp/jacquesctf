/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <limits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ds-file.hpp"
#include "io-error.hpp"

namespace jacques {

DsFile::DsFile(Trace& trace, boost::filesystem::path path) :
    _trace {&trace},
    _path {std::move(path)},
    _factory {
        std::make_unique<yactfr::MemoryMappedFileViewFactory>(_path.string(), 8 << 20,
                                                              yactfr::MemoryMappedFileViewFactory::AccessPattern::SEQUENTIAL)
    },
    _seq {trace.metadata().traceType(), *_factory}
{
    _fileLen = DataLen::fromBytes(boost::filesystem::file_size(_path));
    _fd = open(_path.string().c_str(), O_RDONLY);

    if (_fd < 0) {
        throw IOError {_path, "Cannot open file."};
    }
}

DsFile::~DsFile()
{
    if (_fd >= 0) {
        static_cast<void>(close(_fd));
    }
}

void DsFile::buildIndex()
{
    this->buildIndex([](const auto&) {}, std::numeric_limits<Size>::max());
}

void DsFile::buildIndex(const BuildIndexProgressFunc& progressFunc, const Size step)
{
    if (_isIndexBuilt) {
        return;
    }

    if (_fileLen == 0) {
        _isIndexBuilt = true;
        return;
    }

    const auto oldExpectedAccessPattern = _factory->expectedAccessPattern();

    _factory->expectedAccessPattern(yactfr::MemoryMappedFileViewFactory::AccessPattern::RANDOM);
    this->_buildIndex(progressFunc, step);
    _factory->expectedAccessPattern(oldExpectedAccessPattern);
    _isIndexBuilt = true;
    _pkts.resize(_index.size());
}

void DsFile::_addPktIndexEntry(const Index offsetInDsFileBytes, const Index offsetInDsFileBits,
                               const _IndexBuildingState& state, bool isInvalid)
{
    auto expectedTotalLen = state.expectedTotalLen;
    auto expectedContentLen = state.expectedContentLen;
    DataLen effectiveTotalLen;
    DataLen effectiveContentLen;

    if (!expectedTotalLen) {
        if (expectedContentLen) {
            expectedTotalLen = expectedContentLen;
        } else {
            expectedTotalLen = _fileLen;
        }
    }

    if (!expectedContentLen) {
        expectedContentLen = expectedTotalLen;
    }

    const auto availLen = DataLen::fromBytes(_fileLen.bytes() - offsetInDsFileBytes);

    if (isInvalid) {
        effectiveContentLen = offsetInDsFileBits - offsetInDsFileBytes * 8;

        if (state.expectedTotalLen && *state.expectedTotalLen <= availLen) {
            effectiveTotalLen = *state.expectedTotalLen;
        } else {
            effectiveTotalLen = availLen;
        }
    } else {
        effectiveTotalLen = *expectedTotalLen;
        effectiveContentLen = *expectedContentLen;

        if (effectiveTotalLen > availLen) {
            effectiveTotalLen = availLen;
            isInvalid = true;
        }

        if (effectiveContentLen > effectiveTotalLen || effectiveContentLen > availLen) {
            effectiveContentLen = effectiveTotalLen;
            isInvalid = true;
        }
    }

    if (isInvalid) {
        _hasError = true;
    }

    _index.push_back(PktIndexEntry {
        _index.size(), offsetInDsFileBytes,
        state.pktCtxOffsetInPktBits,
        state.preambleLen,
        expectedTotalLen, expectedContentLen,
        effectiveTotalLen, effectiveContentLen,
        state.dst, state.dsId, state.beginTs, state.endTs, state.seqNum,
        state.discErCounterSnap, isInvalid,
    });
}

void DsFile::_IndexBuildingState::reset()
{
    inPktCtxScope = false;
    preambleLen = boost::none;
    pktCtxOffsetInPktBits = boost::none;
    expectedTotalLen = boost::none;
    expectedContentLen = boost::none;
    beginTs = boost::none;
    endTs = boost::none;
    seqNum = boost::none;
    dsId = boost::none;
    discErCounterSnap = boost::none;
    dst = nullptr;
}

void DsFile::_buildIndex(const BuildIndexProgressFunc& progressFunc, const Size step)
{
    auto it = _seq.begin();
    const auto endIt = _seq.end();
    Index offsetBytes = 0;
    _IndexBuildingState state;
    bool pktStarted = false;
    boost::optional<Ts> curBeginTs;

    try {
        while (it != endIt) {
            switch (it->kind()) {
            case yactfr::Element::Kind::PACKET_BEGINNING:
                offsetBytes = it.offset() / 8;
                pktStarted = true;
                curBeginTs = boost::none;
                break;

            case yactfr::Element::Kind::SCOPE_BEGINNING:
            {
                auto& elem = it->asScopeBeginningElement();

                if (elem.scope() == yactfr::Scope::PACKET_CONTEXT) {
                    state.inPktCtxScope = true;
                    state.pktCtxOffsetInPktBits = it.offset() - (offsetBytes * 8);
                }

                break;
            }

            case yactfr::Element::Kind::PACKET_CONTENT_END:
            case yactfr::Element::Kind::EVENT_RECORD_BEGINNING:
            {
                state.preambleLen = it.offset() - offsetBytes * 8;
                state.inPktCtxScope = false;
                this->_addPktIndexEntry(offsetBytes, it.offset(), state, false);

                if (_index.size() % step == 0) {
                    progressFunc(_index.back());
                }

                state.reset();

                /*
                 * If the effective total length is not the expected
                 * total length, then it covers the whole file anyway,
                 * so `nextOffsetBytes` below will be equal to
                 * _fileLen.bytes().
                 */
                const auto nextOffsetBytes = offsetBytes +
                                             _index.back().effectiveTotalLen().bytes();

                if (nextOffsetBytes >= _fileLen.bytes()) {
                    it = endIt;
                } else {
                    it.seekPacket(nextOffsetBytes);
                }

                continue;
            }

            case yactfr::Element::Kind::PACKET_INFO:
            {
                auto& elem = it->asPacketInfoElement();

                if (elem.expectedTotalLength()) {
                    state.expectedTotalLen = *elem.expectedTotalLength();
                }

                if (elem.expectedContentLength()) {
                    state.expectedContentLen = *elem.expectedContentLength();
                }

                if (_trace->metadata().isCorrelatable()) {
                    assert(!state.beginTs);
                    state.beginTs = curBeginTs;
                }

                if (elem.endDefaultClockValue()) {
                    if (_trace->metadata().isCorrelatable()) {
                        assert(!state.endTs);
                        assert(state.dst);
                        assert(state.dst->defaultClockType());
                        state.endTs = Ts {*elem.endDefaultClockValue(), *state.dst->defaultClockType()};
                    }
                }

                if (elem.sequenceNumber()) {
                    state.seqNum = *elem.sequenceNumber();
                }

                if (elem.discardedEventRecordCounterSnapshot()) {
                    state.discErCounterSnap = *elem.discardedEventRecordCounterSnapshot();
                }

                break;
            }

            case yactfr::Element::Kind::DATA_STREAM_INFO:
            {
                auto& elem = it->asDataStreamInfoElement();

                if (elem.type()) {
                    state.dst = elem.type();
                }

                if (elem.id()) {
                    state.dsId = elem.id();
                }

                break;
            }

            case yactfr::Element::Kind::DEFAULT_CLOCK_VALUE:
            {
                if (state.inPktCtxScope && _trace->metadata().isCorrelatable()) {
                    auto& elem = it->asDefaultClockValueElement();

                    assert(state.dst);
                    assert(state.dst->defaultClockType());
                    curBeginTs = Ts {elem.cycles(), *state.dst->defaultClockType()};
                }

                break;
            }

            default:
                break;
            }

            ++it;
        }
    } catch (const yactfr::DecodingError&) {
        if (pktStarted) {
            /*
             * Error while reading the packet before creating an index
             * entry: create an invalid entry so that we know about
             * this.
             */
            this->_addPktIndexEntry(offsetBytes, it.offset(), state, true);
        }
    }
}

bool DsFile::hasOffsetBits(const Index offsetBits) const noexcept
{
    assert(_isIndexBuilt);

    if (_fileLen == 0) {
        return false;
    }

    assert(!_index.empty());

    return offsetBits < _index.back().endOffsetInDsFileBits();
}

const PktIndexEntry& DsFile::pktIndexEntryContainingOffsetBits(const Index offsetBits) const noexcept
{
    assert(this->hasOffsetBits(offsetBits));

    auto it = std::upper_bound(_index.begin(), _index.end(), offsetBits,
                               [](const auto offsetBits, const auto& entry) {
        return offsetBits < entry.offsetInDsFileBits() + entry.effectiveTotalLen();
    });

    assert(it != _index.end());
    assert(offsetBits >= it->offsetInDsFileBits() &&
           offsetBits < it->offsetInDsFileBits() + it->effectiveTotalLen());


    return *it;
}

const PktIndexEntry *DsFile::pktIndexEntryContainingNsFromOrigin(const long long nsFromOrigin) const noexcept
{
    const auto tsLtCompFunc = [](const Ts& ts, const long long nsFromOrigin) -> bool {
        return ts.nsFromOrigin() < nsFromOrigin;
    };

    const auto valInTsFunc = [](const long long nsFromOrigin, const PktIndexEntry& entry) -> bool {
        return nsFromOrigin >= entry.beginTs()->nsFromOrigin() &&
               nsFromOrigin < entry.endTs()->nsFromOrigin();
    };

    return this->_pktIndexEntryContainingVal(tsLtCompFunc, valInTsFunc, nsFromOrigin);
}

const PktIndexEntry *DsFile::pktIndexEntryContainingCycles(const unsigned long long cycles) const noexcept
{
    const auto tsLtCompFunc = [](const Ts& ts, const unsigned long long cycles) -> bool {
        return ts.cycles() < cycles;
    };

    const auto valInTsFunc = [](const unsigned long long cycles,
                                const PktIndexEntry& entry) -> bool {
        return cycles >= entry.beginTs()->cycles() &&
               cycles < entry.endTs()->cycles();
    };

    return this->_pktIndexEntryContainingVal(tsLtCompFunc, valInTsFunc, cycles);
}

const PktIndexEntry *DsFile::pktIndexEntryWithSeqNum(const Index seqNum) const noexcept
{
    assert(_isIndexBuilt);

    if (_index.empty()) {
        return nullptr;
    }

    auto it = std::upper_bound(_index.begin(), _index.end(), seqNum,
                               [](const auto seqNum, const auto& entry) {
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

Pkt& DsFile::pktAtIndex(const Index index, PktCheckpointsBuildListener& buildListener)
{
    assert(_isIndexBuilt);
    assert(index < _index.size());

    if (!_pkts[index]) {
        auto& pktIndexEntry = _index[index];
        auto mmapFile = std::make_unique<MemMappedFile>(_path, _fd);

        buildListener.startBuild(pktIndexEntry);

        auto pkt = std::make_unique<Pkt>(pktIndexEntry, _seq, _trace->metadata(),
                                         _factory->createDataSource(), std::move(mmapFile),
                                         buildListener);

        buildListener.endBuild();

        if (pkt->error()) {
            pktIndexEntry.isInvalid(true);
        }

        pktIndexEntry.erCount(pkt->erCount());
        _pkts[index] = std::move(pkt);
    }

    return *_pkts[index];
}

} // namespace jacques
