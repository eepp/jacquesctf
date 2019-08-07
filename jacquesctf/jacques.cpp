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
#include <yactfr/metadata/invalid-metadata.hpp>
#include <yactfr/metadata/invalid-metadata-stream.hpp>
#include <yactfr/metadata/metadata-parse-error.hpp>
#include <yactfr/metadata/metadata-stream.hpp>
#include <boost/filesystem.hpp>

#include "config.hpp"
#include "utils.hpp"
#include "print-metadata-text-command.hpp"
#include "list-packets-command.hpp"
#include "copy-packets-command.hpp"
#include "create-lttng-index-command.hpp"
#include "inspect-command.hpp"

namespace bfs = boost::filesystem;

namespace jacques {

static void printCliUsage(const char *cmdName)
{
    std::printf("Usage: %s [GEN OPTS] [CMD] ARG...\n", cmdName);
    std::puts("");
    std::puts("General options:");
    std::puts("");
    std::puts("  --help, -h     Print usage and exit");
    std::puts("  --version, -V  Print version and exit");
    std::puts("");
    std::puts("`inspect` (default) command");
    std::puts("---------------------------");
    std::puts("Usage: inspect PATH...");
    std::puts("");
    std::puts("Interactively inspect CTF traces, CTF data stream files, or CTF metadata");
    std::puts("stream file.");
    std::puts("");
    std::puts("If PATH is a CTF metadata file, print its text content and exit.");
    std::puts("If PATH is a CTF data stream file, inspect this file.");
    std::puts("If PATH is a directory, inspect all CTF data stream files found recursively.");
    std::puts("");
    std::puts("`list-packets` command");
    std::puts("----------------------");
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
    std::puts("----------------------");
    std::puts("Usage: copy-packets SRC-PATH PACKET-INDEXES DST-PATH");
    std::puts("");
    std::puts("Copy specific packets PACKET-INDEXES from CTF data stream file SRC-PATH to new");
    std::puts("CTF data stream file DST-PATH.");
    std::puts("");
    std::puts("PACKET-INDEXES is a single argument containing the indexes of the packets to");
    std::puts("copy from SRC-PATH:");
    std::puts("");
    std::puts("* The first packet is considered to have index 1.");
    std::puts("* The packet indexes are space-delimited.");
    std::puts("* You can copy a range of packets with the `A..B` format (packet A to B,");
    std::puts("  inclusively). A can be greater than B to reverse the copy order.");
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
    std::puts("----------------------------");
    std::puts("Usage: create-lttng-index PATH...");
    std::puts("");
    std::puts("Create an LTTng index file for each specified CTF data stream file.");
    std::puts("");
    std::puts("If PATH is a CTF data stream file, inspect this file.");
    std::puts("If PATH is a directory, inspect all CTF data stream files found recursively.");
}

static void printVersion()
{
    std::cout << "Jacques CTF " JACQUES_VERSION << std::endl;
}

static void jacques(const int argc, const char *argv[])
{
    const auto cfg = configFromArgs(argc, argv);

    if (dynamic_cast<const PrintCliUsageConfig *>(cfg.get())) {
        printCliUsage(argv[0]);
    } else if (dynamic_cast<const PrintVersionConfig *>(cfg.get())) {
        printVersion();
    } else if (const auto specCfg = dynamic_cast<const PrintMetadataTextConfig *>(cfg.get())) {
        printMetadataTextCommand(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const ListPacketsConfig *>(cfg.get())) {
        listPacketsCommand(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CopyPacketsConfig *>(cfg.get())) {
        copyPacketsCommand(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CreateLttngIndexConfig *>(cfg.get())) {
        createLttngIndexCommand(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const InspectConfig *>(cfg.get())) {
        inspectCommand(*specCfg);
    } else {
        std::abort();
    }
}

} // namespace jacques

int main(const int argc, const char *argv[])
{
    const auto exStr = jacques::utils::tryFunc([&]() {
        jacques::jacques(argc, argv);
    });

    if (exStr) {
        jacques::utils::error() << *exStr << std::endl;
        return 1;
    }
}
