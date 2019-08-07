/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <boost/endian/buffers.hpp>

#include "config.hpp"
#include "create-lttng-index-command.hpp"
#include "command-error.hpp"
#include "metadata.hpp"
#include "data-stream-file.hpp"

namespace bfs = boost::filesystem;
namespace bendian = boost::endian;

namespace jacques {

struct lttngIndexHeader {
    bendian::big_uint32_buf_t magic;
    bendian::big_uint32_buf_t indexMajor;
    bendian::big_uint32_buf_t indexMinor;
    bendian::big_uint32_buf_t indexEntrySizeBytes;
};

struct lttngIndexEntryBase {
    bendian::big_uint64_buf_t offsetBytes;
    bendian::big_uint64_buf_t totalSizeBits;
    bendian::big_uint64_buf_t contentSizeBits;
    bendian::big_uint64_buf_t beginningTimestamp;
    bendian::big_uint64_buf_t endTimestamp;
    bendian::big_uint64_buf_t discardedEventRecordCounter;
    bendian::big_uint64_buf_t dstId;
};

struct lttngIndexEntry11Addon {
    bendian::big_uint64_buf_t dsId;
    bendian::big_uint64_buf_t seqNum;
};

static_assert(sizeof(lttngIndexHeader) == 4 * 4,
              "LTTng index header structure has the expected size.");
static_assert(sizeof(lttngIndexEntryBase) == 7 * 8,
              "LTTng index entry base structure has the expected size.");
static_assert(sizeof(lttngIndexEntry11Addon) == 2 * 8,
              "LTTng index entry v1.1 addon structure has the expected size.");

static bool entryHas11Addon(const DataStreamFile& dsf)
{
    return dsf.packetIndexEntry(0).dataStreamId() &&
           dsf.packetIndexEntry(0).seqNum();
}

static void writeLttngIndexHeader(std::ofstream& idxStream,
                                  const DataStreamFile& dsf)
{
    lttngIndexHeader header;

    header.magic = 0xc1f1dcc1U;
    header.indexMajor = 1;
    header.indexMinor = 0;
    header.indexEntrySizeBytes = sizeof(lttngIndexEntryBase);

    if (entryHas11Addon(dsf)) {
        header.indexMinor = 1;
        header.indexEntrySizeBytes = header.indexEntrySizeBytes.value() +
                                     sizeof(lttngIndexEntry11Addon);
    }

    idxStream.write(reinterpret_cast<char *>(&header), sizeof(header));
}

static void writeLttngIndexEntry(std::ofstream& idxStream,
                                 const DataStreamFile& dsf,
                                 const PacketIndexEntry& indexEntry)
{
    lttngIndexEntryBase entryBase;

    entryBase.offsetBytes = indexEntry.offsetInDataStreamFileBytes();
    entryBase.totalSizeBits = indexEntry.effectiveTotalSize().bits();
    entryBase.contentSizeBits = indexEntry.effectiveContentSize().bits();
    entryBase.beginningTimestamp = 0;

    if (indexEntry.beginningTimestamp()) {
        entryBase.beginningTimestamp = indexEntry.beginningTimestamp()->cycles();
    }

    entryBase.endTimestamp = 0;

    if (indexEntry.endTimestamp()) {
        entryBase.endTimestamp = indexEntry.endTimestamp()->cycles();
    }

    entryBase.discardedEventRecordCounter = 0;

    if (indexEntry.discardedEventRecordCounter()) {
        entryBase.discardedEventRecordCounter = *indexEntry.discardedEventRecordCounter();
    }

    entryBase.dstId = 0;

    if (indexEntry.dataStreamType()) {
        entryBase.dstId = indexEntry.dataStreamType()->id();
    }

    idxStream.write(reinterpret_cast<char *>(&entryBase), sizeof(entryBase));

    if (entryHas11Addon(dsf)) {
        lttngIndexEntry11Addon addon;

        addon.dsId = *indexEntry.dataStreamId();
        addon.seqNum = *indexEntry.seqNum();
        idxStream.write(reinterpret_cast<char *>(&addon), sizeof(addon));
    }
}

static void createDataStreamFileLttngIndex(const DataStreamFile& dsf)
{
    const auto indexDir = dsf.path().parent_path() / "index";

    bfs::create_directories(indexDir);

    const auto idxFilePath = indexDir / (dsf.path().filename().string() + ".idx");

    std::ofstream idxStream;

    idxStream.exceptions(std::ios::badbit | std::ios::failbit);

    try {
        idxStream.open(idxFilePath.c_str(), std::ios::binary);

        writeLttngIndexHeader(idxStream, dsf);

        for (const auto& indexEntry : dsf.packetIndexEntries()) {
            writeLttngIndexEntry(idxStream, dsf, indexEntry);
        }

        idxStream.close();
    } catch (const std::ios_base::failure& ex) {
        throw CommandError {ex.what()};
    }
}

void createLttngIndexCommand(const CreateLttngIndexConfig& cfg)
{
    // metadata file path to metadata
    std::map<bfs::path, std::unique_ptr<const Metadata>> metadatas;

    for (const auto& dsfPath : cfg.paths()) {
        const auto metadataPath = dsfPath.parent_path() / "metadata";
        const auto it = metadatas.find(metadataPath);
        const Metadata *metadata = nullptr;

        if (it == std::end(metadatas)) {
            // create one
            auto metadataUp = std::make_unique<const Metadata>(metadataPath);

            metadata = metadataUp.get();
            metadatas[metadataPath] = std::move(metadataUp);
        } else {
            // already exists
            metadata = it->second.get();
        }

        assert(metadata);

        DataStreamFile dsf {dsfPath, *metadata};

        dsf.buildIndex();
        createDataStreamFileLttngIndex(dsf);
    }
}

} // namespace jacques
