/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILE_HPP
#define _JACQUES_DATA_STREAM_FILE_HPP

#include <cassert>
#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/core/noncopyable.hpp>
#include <yactfr/element-sequence.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/memory-mapped-file-view-factory.hpp>

#include "aliases.hpp"
#include "packet.hpp"
#include "packet-index-entry.hpp"
#include "metadata.hpp"
#include "data-size.hpp"
#include "packet-checkpoints-build-listener.hpp"

namespace jacques {

class DataStreamFile :
    boost::noncopyable
{
public:
    using BuildIndexProgressFunc = std::function<void (const PacketIndexEntry&)>;

public:
    explicit DataStreamFile(const boost::filesystem::path& path,
                            const Metadata& metadata);
    ~DataStreamFile();
    void buildIndex();
    void buildIndex(const BuildIndexProgressFunc& progressFunc,
                    Size step = 1);
    bool hasOffsetBits(Index offsetBits);
    Packet& packetAtIndex(Index index, PacketCheckpointsBuildListener& buildListener);
    const PacketIndexEntry& packetIndexEntryContainingOffsetBits(Index offsetBits);
    const PacketIndexEntry *packetIndexEntryWithSeqNum(Index seqNum);
    const PacketIndexEntry *packetIndexEntryContainingNsFromOrigin(long long nsFromOrigin);
    const PacketIndexEntry *packetIndexEntryContainingCycles(unsigned long long cycles);

    Size packetCount() const noexcept
    {
        assert(_isIndexBuilt);
        return _index.size();
    }

    const PacketIndexEntry& packetIndexEntry(const Index index) const
    {
        assert(_isIndexBuilt);
        assert(index < _index.size());
        return _index[index];
    }

    PacketIndexEntry& packetIndexEntry(const Index index)
    {
        assert(_isIndexBuilt);
        assert(index < _index.size());
        return _index[index];
    }

    const std::vector<PacketIndexEntry>& packetIndexEntries() const noexcept
    {
        assert(_isIndexBuilt);
        return _index;
    }

    std::vector<PacketIndexEntry>& packetIndexEntries() noexcept
    {
        assert(_isIndexBuilt);
        return _index;
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const DataSize& fileSize() const noexcept
    {
        return _fileSize;
    }

    bool hasError() const noexcept
    {
        return _hasError;
    }

    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

private:
    struct _IndexBuildingState
    {
        void reset();

        boost::optional<Index> packetContextOffsetInPacketBits;
        boost::optional<DataSize> preambleSize;
        boost::optional<DataSize> expectedTotalSize;
        boost::optional<DataSize> expectedContentSize;
        boost::optional<Timestamp> tsBegin;
        boost::optional<Timestamp> tsEnd;
        boost::optional<Index> dataStreamId;
        boost::optional<Index> seqNum;
        boost::optional<Size> discardedEventRecordCounter;
        const yactfr::DataStreamType *dst = nullptr;
        bool inPacketContextScope = false;
    };

private:
    void _buildIndex(const BuildIndexProgressFunc& progressFunc,
                     Size step);
    void _addPacketIndexEntry(Index offsetInDataStreamFileBytes,
                              Index offsetInDataStreamFileBits,
                              const _IndexBuildingState& state,
                              bool isInvalid);

    template <typename TsLtCompFuncT, typename ValueInTsFuncT, typename ValueT>
    const PacketIndexEntry *_packetIndexEntryContainingValue(TsLtCompFuncT&& tsLtCompFunc,
                                                             ValueInTsFuncT&& valueInTsFunc,
                                                             const ValueT value)
    {
        if (!_metadata->isCorrelatable()) {
            return nullptr;
        }

        if (_index.empty()) {
            return nullptr;
        }

        auto it = std::lower_bound(std::begin(_index), std::end(_index),
                                   value, [tsLtCompFunc](const auto& entry,
                                                         const auto value) {
            if (!entry.beginningTimestamp()) {
                return false;
            }

            return std::forward<TsLtCompFuncT>(tsLtCompFunc)(*entry.beginningTimestamp(),
                                                             value);
        });

        if (it == std::end(_index)) {
            --it;
        }

        if (!it->beginningTimestamp() || !it->endTimestamp()) {
            return nullptr;
        }

        if (!std::forward<ValueInTsFuncT>(valueInTsFunc)(value, *it)) {
            if (it == std::begin(_index)) {
                return nullptr;
            }

            --it;

            if (!it->beginningTimestamp() || !it->endTimestamp()) {
                return nullptr;
            }

            if (!std::forward<ValueInTsFuncT>(valueInTsFunc)(value, *it)) {
                return nullptr;
            }
        }

        return &*it;
    }

private:
    const boost::filesystem::path _path;
    const Metadata * const _metadata;
    std::shared_ptr<yactfr::MemoryMappedFileViewFactory> _factory;
    yactfr::ElementSequence _seq;
    DataSize _fileSize;
    std::vector<PacketIndexEntry> _index;
    std::vector<std::unique_ptr<Packet>> _packets;
    int _fd;
    bool _isIndexBuilt = false;
    bool _hasError = false;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_HPP
