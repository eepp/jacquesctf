/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cassert>

#include "config.hpp"
#include "list-packets-command.hpp"
#include "data/metadata.hpp"
#include "data/data-stream-file.hpp"
#include "data/time-ops.hpp"

namespace jacques {

static void printHeader(const ListPacketsConfig::Format fmt)
{
    assert(fmt == ListPacketsConfig::Format::MACHINE);

    std::cout << "Index,Offset (bytes),Total size (bytes)," <<
                 "Content size (bits),Beginning time (cycles)," <<
                 "Beginning time (ns),End time (cycles),End time (ns)," <<
                 "Duration (cycles),Duration (ns),Data stream type ID," <<
                 "Data stream ID,Sequence number," <<
                 "Discarded event record counter,Is valid" << std::endl;
}

static void printRow(const PacketIndexEntry& indexEntry,
                     const ListPacketsConfig::Format fmt)
{
    assert(fmt == ListPacketsConfig::Format::MACHINE);

    std::cout << indexEntry.natIndexInDataStreamFile() << "," <<
                 indexEntry.offsetInDataStreamFileBytes() << "," <<
                 indexEntry.effectiveTotalSize().bytes() << "," <<
                 indexEntry.effectiveContentSize().bits() << ",";

    if (indexEntry.beginningTimestamp()) {
        std::cout << indexEntry.beginningTimestamp()->cycles() << "," <<
                     indexEntry.beginningTimestamp()->nsFromOrigin() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.endTimestamp()) {
        std::cout << indexEntry.endTimestamp()->cycles() << "," <<
                     indexEntry.endTimestamp()->nsFromOrigin() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.beginningTimestamp() && indexEntry.endTimestamp() &&
            *indexEntry.beginningTimestamp() <= *indexEntry.endTimestamp()) {
        const auto duration = *indexEntry.endTimestamp() -
                              *indexEntry.beginningTimestamp();
        const auto durCycles = indexEntry.endTimestamp()->cycles() -
                               indexEntry.beginningTimestamp()->cycles();

        std::cout << durCycles << "," <<
                     duration.ns() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.dataStreamType()) {
        std::cout << indexEntry.dataStreamType()->id() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.dataStreamId()) {
        std::cout << *indexEntry.dataStreamId() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.seqNum()) {
        std::cout << *indexEntry.seqNum() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.discardedEventRecordCounter()) {
        std::cout << *indexEntry.discardedEventRecordCounter() << ",";
    } else {
        std::cout << ",";
    }

    std::cout << (indexEntry.isInvalid() ? "no" : "yes") << std::endl;
}

void listPacketsCommand(const ListPacketsConfig& cfg)
{
    const Metadata metadata {cfg.path().parent_path() / "metadata"};
    DataStreamFile dsf {cfg.path(), metadata};

    dsf.buildIndex();

    if (dsf.packetCount() == 0) {
        // nothing to print
        return;
    }

    if (cfg.withHeader()) {
        printHeader(cfg.format());
    }

    for (const auto& indexEntry : dsf.packetIndexEntries()) {
        printRow(indexEntry, cfg.format());
    }
}

} // namespace jacques
