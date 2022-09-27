/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
#include <set>
#include <boost/endian/buffers.hpp>

#include "cfg.hpp"
#include "create-lttng-index-cmd.hpp"
#include "cmd-error.hpp"
#include "data/trace.hpp"
#include "data/metadata.hpp"
#include "data/ds-file.hpp"

namespace jacques {

namespace bfs = boost::filesystem;
namespace bendian = boost::endian;

namespace {

struct LTTngIndexHeader {
    bendian::big_uint32_buf_t magic;
    bendian::big_uint32_buf_t indexMajor;
    bendian::big_uint32_buf_t indexMinor;
    bendian::big_uint32_buf_t indexEntrySizeBytes;
};

struct LTTngIndexEntryBase {
    bendian::big_uint64_buf_t offsetBytes;
    bendian::big_uint64_buf_t totalLenBits;
    bendian::big_uint64_buf_t contentLenBits;
    bendian::big_uint64_buf_t beginTs;
    bendian::big_uint64_buf_t endTs;
    bendian::big_uint64_buf_t discErCounterSnap;
    bendian::big_uint64_buf_t dstId;
};

struct LTTngIndexEntry11Addon {
    bendian::big_uint64_buf_t dsId;
    bendian::big_uint64_buf_t seqNum;
};

static_assert(sizeof(LTTngIndexHeader) == 4 * 4,
              "LTTng index header structure has the expected size.");
static_assert(sizeof(LTTngIndexEntryBase) == 7 * 8,
              "LTTng index entry base structure has the expected size.");
static_assert(sizeof(LTTngIndexEntry11Addon) == 2 * 8,
              "LTTng index entry v1.1 addon structure has the expected size.");

bool entryHas11Addon(const DsFile& dsf) noexcept
{
    return dsf.pktIndexEntry(0).dsId() && dsf.pktIndexEntry(0).seqNum();
}

template <typename DataT>
void writeData(std::ofstream& os, const DataT& data)
{
    os.write(reinterpret_cast<const char *>(&data), sizeof(data));
}

void writeLTTngIndexHeader(std::ofstream& idxStream, const DsFile& dsf)
{
    LTTngIndexHeader header;

    header.magic = 0xc1f1dcc1U;
    header.indexMajor = 1;
    header.indexMinor = 0;
    header.indexEntrySizeBytes = sizeof(LTTngIndexEntryBase);

    if (entryHas11Addon(dsf)) {
        header.indexMinor = 1;
        header.indexEntrySizeBytes = header.indexEntrySizeBytes.value() +
                                     sizeof(LTTngIndexEntry11Addon);
    }

    writeData(idxStream, header);
}

void writeLttngIndexEntry(std::ofstream& idxStream, const DsFile& dsf,
                          const PktIndexEntry& indexEntry)
{
    LTTngIndexEntryBase entryBase;

    entryBase.offsetBytes = indexEntry.offsetInDsFileBytes();
    entryBase.totalLenBits = indexEntry.effectiveTotalLen().bits();
    entryBase.contentLenBits = indexEntry.effectiveContentLen().bits();
    entryBase.beginTs = 0;

    if (indexEntry.beginTs()) {
        entryBase.beginTs = indexEntry.beginTs()->cycles();
    }

    entryBase.endTs = 0;

    if (indexEntry.endTs()) {
        entryBase.endTs = indexEntry.endTs()->cycles();
    }

    entryBase.discErCounterSnap = 0;

    if (indexEntry.discErCounterSnap()) {
        entryBase.discErCounterSnap = *indexEntry.discErCounterSnap();
    }

    entryBase.dstId = 0;

    if (indexEntry.dst()) {
        entryBase.dstId = indexEntry.dst()->id();
    }

    writeData(idxStream, entryBase);

    if (entryHas11Addon(dsf)) {
        LTTngIndexEntry11Addon addon;

        addon.dsId = *indexEntry.dsId();
        addon.seqNum = *indexEntry.seqNum();
        writeData(idxStream, addon);
    }
}

void createDsFileLttngIndex(const DsFile& dsf)
{
    const auto indexDir = dsf.path().parent_path() / "index";

    bfs::create_directories(indexDir);

    const auto idxFilePath = indexDir / (dsf.path().filename().string() + ".idx");

    std::ofstream idxStream;

    idxStream.exceptions(std::ios::badbit | std::ios::failbit);

    try {
        idxStream.open(idxFilePath.c_str(), std::ios::binary);

        writeLTTngIndexHeader(idxStream, dsf);

        for (const auto& indexEntry : dsf.pktIndexEntries()) {
            writeLttngIndexEntry(idxStream, dsf, indexEntry);
        }

        idxStream.close();
    } catch (const std::ios_base::failure& exc) {
        throw CmdError {exc.what()};
    }
}

} // namespace

void createLttngIndexCmd(const CreateLttngIndexCfg& cfg)
{
    // trace directory to set of data stream file paths
    std::map<bfs::path, std::vector<bfs::path>> groupedDsFilePaths;

    for (auto& dsfPath : cfg.paths()) {
        groupedDsFilePaths[dsfPath.parent_path()].push_back(dsfPath);
    }

    // create indexes
    for (const auto& traceDirDsFilePathsPair : groupedDsFilePaths) {
        // create trace with specific data stream files
        Trace trace {traceDirDsFilePathsPair.second};

        for (auto& dsf : trace.dsFiles()) {
            dsf->buildIndex();
            createDsFileLttngIndex(*dsf);
        }
    }
}

} // namespace jacques
