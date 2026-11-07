/*
 * SPDX-FileCopyrightText: 2019 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <fmt/format.h>

#include "cfg.hpp"
#include "list-pkts-cmd.hpp"
#include "data/trace.hpp"
#include "data/ds-file.hpp"
#include "data/time-ops.hpp"

namespace jacques {
namespace {

void printHeader([[maybe_unused]] const ListPktsCfg::Fmt outFmt, const bool withErCount)
{
    assert(outFmt == ListPktsCfg::Fmt::MACHINE);

    fmt::print("Index,Offset (bytes),Total length (bytes),"
               "Content length (bits),Beginning time (cycles),"
               "Beginning timestamp (ns),End timestamp (cycles),End timestamp (ns),"
               "Duration (cycles),Duration (ns),Data stream type ID,"
               "Data stream ID,Sequence number,"
               "Discarded event record counter snapshot,");

    if (withErCount) {
        fmt::print("Event record count,");
    }

    fmt::print("Is valid?\n");
}

void printRow(const PktIndexEntry& indexEntry, [[maybe_unused]] const ListPktsCfg::Fmt outFmt,
              const bool withErCount)
{
    assert(outFmt == ListPktsCfg::Fmt::MACHINE);

    fmt::print("{},{},{},{},", indexEntry.natIndexInDsFile(), indexEntry.offsetInDsFileBytes(),
               indexEntry.effectiveTotalLen().bytes(), indexEntry.effectiveContentLen().bits());

    if (indexEntry.beginTs()) {
        fmt::print("{},{},", indexEntry.beginTs()->cycles(), indexEntry.beginTs()->nsFromOrigin());
    } else {
        fmt::print(",,");
    }

    if (indexEntry.endTs()) {
        fmt::print("{},{},", indexEntry.endTs()->cycles(), indexEntry.endTs()->nsFromOrigin());
    } else {
        fmt::print(",,");
    }

    if (indexEntry.beginTs() && indexEntry.endTs() &&
            *indexEntry.beginTs() <= *indexEntry.endTs()) {
        const auto duration = *indexEntry.endTs() -
                              *indexEntry.beginTs();
        const auto durCycles = indexEntry.endTs()->cycles() -
                               indexEntry.beginTs()->cycles();

        fmt::print("{},{},", durCycles, duration.ns());
    } else {
        fmt::print(",,");
    }

    if (indexEntry.dst()) {
        fmt::print("{},", indexEntry.dst()->id());
    } else {
        fmt::print(",");
    }

    if (indexEntry.dsId()) {
        fmt::print("{},", *indexEntry.dsId());
    } else {
        fmt::print(",");
    }

    if (indexEntry.seqNum()) {
        fmt::print("{},", *indexEntry.seqNum());
    } else {
        fmt::print(",");
    }

    if (indexEntry.discErCounterSnap()) {
        fmt::print("{},", *indexEntry.discErCounterSnap());
    } else {
        fmt::print(",");
    }

    if (withErCount) {
        // no event record count for an invalid packet
        if (!indexEntry.isInvalid() && indexEntry.erCount()) {
            fmt::print("{}", *indexEntry.erCount());
        }

        fmt::print(",");
    }

    fmt::print("{}\n", indexEntry.isInvalid() ? "no" : "yes");
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
