/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <yactfr/yactfr.hpp>
#include <boost/filesystem.hpp>

#include "cfg.hpp"
#include "utils.hpp"
#include "print-metadata-text-cmd.hpp"
#include "list-pkts-cmd.hpp"
#include "copy-pkts-cmd.hpp"
#include "create-lttng-index-cmd.hpp"
#include "inspect-cmd/ui/inspect-cmd.hpp"

namespace bfs = boost::filesystem;

namespace jacques {

static void printCliUsage(const char * const cmdName)
{
    std::printf("Usage: %s [GEN OPTS] [CMD] ARG...\n", cmdName);
    std::puts("");
    std::puts("General options:");
    std::puts("");
    std::puts("  --help, -h     Print usage and exit");
    std::puts("  --version, -V  Print version and exit");
    std::puts("");
    std::puts("`inspect` command (default)");
    std::puts("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
    std::puts("Usage: inspect PATH...");
    std::puts("");
    std::puts("Interactively inspect CTF traces, CTF data stream files, or CTF metadata");
    std::puts("stream files.");
    std::puts("");
    std::puts("If PATH is a single CTF metadata file, print its text content and exit.");
    std::puts("If PATH is a CTF data stream file, inspect this file.");
    std::puts("If PATH is a directory, inspect all CTF data stream files found recursively.");
    std::puts("");
    std::puts("`list-packets` command");
    std::puts("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
    std::puts("Usage: list-packets --machine [--header] PATH");
    std::puts("");
    std::puts("Print the list of packets of CTF data stream file PATH and their properties.");
    std::puts("");
    std::puts("Options:");
    std::puts("");
    std::puts("  --header       Print table header");
    std::puts("  --machine, -m  Print machine-readable data (CSV)");
    std::puts("");
    std::puts("`copy-packets` command");
    std::puts("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
    std::puts("Usage: copy-packets SRC-PATH PACKET-INDEXES DST-PATH");
    std::puts("");
    std::puts("Copy the specific packets PACKET-INDEXES from the CTF data stream file SRC-PATH");
    std::puts("to the new CTF data stream file DST-PATH.");
    std::puts("");
    std::puts("PACKET-INDEXES is a single argument containing the indexes of the packets to");
    std::puts("copy from SRC-PATH:");
    std::puts("");
    std::puts("* The first packet is considered to have index 1.");
    std::puts("");
    std::puts("* The packet indexes are space-delimited.");
    std::puts("");
    std::puts("* You can copy a range of packets with the `A..B` format (packet A to B,");
    std::puts("  inclusively). A can be greater than B to reverse the copy order.");
    std::puts("");
    std::puts("* You can copy the Nth last packet with the `:N` format, where `:1` means the");
    std::puts("  last packet.");
    std::puts("");
    std::puts("Examples:");
    std::puts("");
    std::puts("    17");
    std::puts("    1 2 3 9 10");
    std::puts("    2 3 7..11 16..21 :4 :1");
    std::puts("    2 9 :10..:5");
    std::puts("    1 :5..4 99");
    std::puts("    1..:1");
    std::puts("    2..:1");
    std::puts("    1..:2");
    std::puts("");
    std::puts("`create-lttng-index` command");
    std::puts("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
    std::puts("Usage: create-lttng-index PATH...");
    std::puts("");
    std::puts("Create an LTTng index file for each specified CTF data stream file.");
    std::puts("");
    std::puts("If PATH is a CTF data stream file, use this file.");
    std::puts("If PATH is a directory, use all the CTF data stream files found recursively.");
}

static void printVersion()
{
    std::cout << "Jacques CTF " JACQUES_VERSION << std::endl;
}

static void jacques(const int argc, const char *argv[])
{
    const auto cfg = cfgFromArgs(argc, argv);

    if (dynamic_cast<const PrintCliUsageCfg *>(cfg.get())) {
        printCliUsage(argv[0]);
    } else if (dynamic_cast<const PrintVersionCfg *>(cfg.get())) {
        printVersion();
    } else if (const auto specCfg = dynamic_cast<const PrintMetadataTextCfg *>(cfg.get())) {
        printMetadataTextCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const ListPktsCfg *>(cfg.get())) {
        listPktsCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CopyPktsCfg *>(cfg.get())) {
        copyPktsCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CreateLttngIndexCfg *>(cfg.get())) {
        createLttngIndexCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const InspectCfg *>(cfg.get())) {
        inspectCmd(*specCfg);
    } else {
        std::abort();
    }
}

} // namespace jacques

int main(const int argc, const char *argv[])
{
    const auto exStr = jacques::utils::tryFunc([argc, argv] {
        jacques::jacques(argc, argv);
    });

    if (exStr) {
        jacques::utils::error() << *exStr << std::endl;
        return 1;
    }
}
