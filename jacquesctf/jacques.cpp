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
    std::puts("`inspect` (default) command usage: PATH...");
    std::puts("");
    std::puts("  Interactively inspect traces, data stream files, or metadata stream file.");
    std::puts("");
    std::puts("  If PATH is a CTF metadata file, print its text content and exit.");
    std::puts("  If PATH is a CTF data stream file, inspect this file.");
    std::puts("  If PATH is a directory, inspect all CTF data stream files found recursively.");
    std::puts("");
    std::puts("`list-packets` command usage: --machine [--header] PATH");
    std::puts("");
    std::puts("  Print the list of packets of data stream file PATH and their properties.");
    std::puts("");
    std::puts("  --header       Print table header");
    std::puts("  --machine, -m  Print machine-readable data (CSV)");
    std::puts("");
    std::puts("`copy-packets` command usage: SRC-PATH PACKET-INDEXES DST-PATH");
    std::puts("");
    std::puts("  Copy specific packets PACKET-INDEXES from data stream file SRC-PATH to new");
    std::puts("  data stream file DST-PATH.");
    std::puts("");
    std::puts("  PACKET-INDEXES is a single argument containing the indexes of the packets to");
    std::puts("  copy from SRC-PATH:");
    std::puts("");
    std::puts("  * The first packet is considered to have index 1.");
    std::puts("  * The packet indexes are space-delimited.");
    std::puts("  * You can copy a range of packets with the `A..B` format (packet A to B,");
    std::puts("    inclusively). A can be greater than B to reverse the copy order.");
    std::puts("  * You can copy the Nth last packet with the `:N` format, where `:1` means the");
    std::puts("    last packet.");
    std::puts("");
    std::puts("  Examples:");
    std::puts("");
    std::puts("      17");
    std::puts("      1 2 3 9 10");
    std::puts("      2 3 7..11 16..21 :4 :1");
    std::puts("      2 9 :10..:5");
    std::puts("      1 :5..4 99");
    std::puts("      1..:1");
    std::puts("      2..:1");
    std::puts("      1..:2");
}

static void printVersion()
{
    std::cout << "Jacques CTF " JACQUES_VERSION << std::endl;
}

static bool jacques(const int argc, const char *argv[])
{
    auto cfg = configFromArgs(argc, argv);

    if (!cfg) {
        return false;
    }

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
    } else if (const auto specCfg = dynamic_cast<const InspectConfig *>(cfg.get())) {
        inspectCommand(*specCfg);
    } else {
        std::abort();
    }

    return true;
}

} // namespace jacques

int main(const int argc, const char *argv[])
{
    bool ret;

    const auto exStr = jacques::utils::tryFunc([&]() {
        ret = jacques::jacques(argc, argv) ? 0 : 1;
    });

    if (exStr) {
        jacques::utils::error() << *exStr << std::endl;
        return 1;
    }

    return ret ? 0 : 1;
}
