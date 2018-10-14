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

namespace jacques {

class DataStreamFile
{
public:
    using BuildIndexProgressFunc = std::function<void (const PacketIndexEntry&)>;

public:
    explicit DataStreamFile(const boost::filesystem::path& path,
                            const Metadata& metadata,
                            yactfr::PacketSequence& seq,
                            yactfr::MemoryMappedFileViewFactory& factory);
    void buildIndex(const BuildIndexProgressFunc& progressFunc,
                    Size step = 1);

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
        return *_path;
    }

    const DataSize& fileSize() const noexcept
    {
        return _fileSize;
    }

    bool isComplete() const noexcept
    {
        return _isComplete;
    }

private:
    struct _IndexBuildingState
    {
        boost::optional<Index> packetContextOffsetInPacketBits;
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
    void _resetIndexBuildingState(_IndexBuildingState& state);
    void _buildIndex(const BuildIndexProgressFunc& progressFunc,
                     Size step);
    void _addPacketIndexEntry(Index offsetInDataStreamBytes,
                              const _IndexBuildingState& state,
                              bool isInvalid);

private:
    const boost::filesystem::path *_path;
    const Metadata *_metadata;
    yactfr::PacketSequence *_seq;
    yactfr::MemoryMappedFileViewFactory *_factory;
    DataSize _fileSize;
    std::vector<PacketIndexEntry> _index;
    bool _isIndexBuilt = false;
    bool _isComplete = true;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_HPP
