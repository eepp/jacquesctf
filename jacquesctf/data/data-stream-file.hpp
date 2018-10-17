/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILE_HPP
#define _JACQUES_DATA_STREAM_FILE_HPP

#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <yactfr/packet-sequence.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/memory-mapped-file-view-factory.hpp>

#include "aliases.hpp"
#include "packet.hpp"
#include "packet-index-entry.hpp"
#include "metadata.hpp"
#include "data-size.hpp"
#include "packet-checkpoints-build-listener.hpp"

namespace jacques {

class DataStreamFile
{
public:
    using BuildIndexProgressFunc = std::function<void (const PacketIndexEntry&)>;

public:
    explicit DataStreamFile(const boost::filesystem::path& path,
                            std::shared_ptr<const Metadata> metadata);
    ~DataStreamFile();
    void buildIndex(const BuildIndexProgressFunc& progressFunc,
                    Size step = 1);
    bool hasOffsetBits(Index offsetBits);
    Packet& packetAtIndex(Index index, PacketCheckpointsBuildListener& buildListener);
    const PacketIndexEntry& packetIndexEntryContainingOffsetBits(Index offsetBits);
    const PacketIndexEntry *packetIndexEntryWithSeqNum(Index seqNum);
    const PacketIndexEntry *packetIndexEntryContainingTimestamp(const Timestamp& ts);

    Size packetCount() const
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
        return _index;
    }

    std::vector<PacketIndexEntry>& packetIndexEntries() noexcept
    {
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

    bool isComplete() const noexcept
    {
        return _isComplete;
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
    void _addPacketIndexEntry(Index offsetInDataStreamBytes,
                              const _IndexBuildingState& state,
                              bool isInvalid);

private:
    const boost::filesystem::path _path;
    std::shared_ptr<const Metadata> _metadata;
    std::shared_ptr<yactfr::MemoryMappedFileViewFactory> _factory;
    yactfr::PacketSequence _seq;
    DataSize _fileSize;
    std::vector<PacketIndexEntry> _index;
    std::vector<std::unique_ptr<Packet>> _packets;
    int _fd;
    bool _isIndexBuilt = false;
    bool _isComplete = true;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_HPP
