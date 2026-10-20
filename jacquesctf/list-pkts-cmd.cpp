/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cassert>

#include "cfg.hpp"
#include "list-pkts-cmd.hpp"
#include "data/trace.hpp"
#include "data/ds-file.hpp"
#include "data/time-ops.hpp"

namespace jacques {
namespace {

void printHeader(const ListPktsCfg::Fmt fmt __attribute__((unused)), const bool withErCount)
{
    assert(fmt == ListPktsCfg::Fmt::MACHINE);

    std::cout << "Index,Offset (bytes),Total length (bytes)," <<
                 "Content length (bits),Beginning time (cycles)," <<
                 "Beginning timestamp (ns),End timestamp (cycles),End timestamp (ns)," <<
                 "Duration (cycles),Duration (ns),Data stream type ID," <<
                 "Data stream ID,Sequence number," <<
                 "Discarded event record counter snapshot,";

    if (withErCount) {
        std::cout << "Event record count,";
    }

    std::cout << "Is valid?" << std::endl;
}

void printRow(const PktIndexEntry& indexEntry, const ListPktsCfg::Fmt fmt __attribute__((unused)),
              const bool withErCount)
{
    assert(fmt == ListPktsCfg::Fmt::MACHINE);

    std::cout << indexEntry.natIndexInDsFile() << "," <<
                 indexEntry.offsetInDsFileBytes() << "," <<
                 indexEntry.effectiveTotalLen().bytes() << "," <<
                 indexEntry.effectiveContentLen().bits() << ",";

    if (indexEntry.beginTs()) {
        std::cout << indexEntry.beginTs()->cycles() << "," <<
                     indexEntry.beginTs()->nsFromOrigin() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.endTs()) {
        std::cout << indexEntry.endTs()->cycles() << "," <<
                     indexEntry.endTs()->nsFromOrigin() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.beginTs() && indexEntry.endTs() &&
            *indexEntry.beginTs() <= *indexEntry.endTs()) {
        const auto duration = *indexEntry.endTs() -
                              *indexEntry.beginTs();
        const auto durCycles = indexEntry.endTs()->cycles() -
                               indexEntry.beginTs()->cycles();

        std::cout << durCycles << "," << duration.ns() << ",";
    } else {
        std::cout << ",,";
    }

    if (indexEntry.dst()) {
        std::cout << indexEntry.dst()->id() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.dsId()) {
        std::cout << *indexEntry.dsId() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.seqNum()) {
        std::cout << *indexEntry.seqNum() << ",";
    } else {
        std::cout << ",";
    }

    if (indexEntry.discErCounterSnap()) {
        std::cout << *indexEntry.discErCounterSnap() << ",";
    } else {
        std::cout << ",";
    }

    if (withErCount) {
        // no event record count for an invalid packet
        if (!indexEntry.isInvalid() && indexEntry.erCount()) {
            std::cout << *indexEntry.erCount();
        }

        std::cout << ",";
    }

    std::cout << (indexEntry.isInvalid() ? "no" : "yes") << std::endl;
}

} // namespace

void listPktsCmd(const ListPktsCfg& cfg)
{
    Trace trace {{cfg.path()}};
    auto& dsf = *trace.dsFiles().front();

    dsf.buildIndex();

    if (dsf.pktCount() == 0) {
        // nothing to print
        return;
    }

    if (cfg.withHeader()) {
        printHeader(cfg.format(), cfg.withErCount());
    }

    for (const auto& indexEntry : dsf.pktIndexEntries()) {
        /*
         * Counting the event records of a packet means decoding it
         * completely, which is slow: only do it on demand.
         *
         * Don't bother with a packet which is already invalid.
         *
         * dsf.countPktErs() marks the packet as invalid on
         * decoding error.
         */
        if (cfg.withErCount() && !indexEntry.isInvalid()) {
            dsf.countPktErs(indexEntry.indexInDsFile());
        }

        printRow(indexEntry, cfg.format(), cfg.withErCount());
    }
}

} // namespace jacques
