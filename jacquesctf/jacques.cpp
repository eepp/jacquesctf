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
#include "interactive.hpp"
#include "utils.hpp"
#include "metadata.hpp"

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
    std::puts("  If PATH is a CTF metadata file, print its text content and exit.");
    std::puts("  If PATH is a CTF data stream file, inspect this file.");
    std::puts("  If PATH is a directory, inspect all CTF data stream files found recursively.");
}

static void printVersion()
{
    std::cout << "Jacques CTF " JACQUES_VERSION << std::endl;
}

static void printMetadataText(const PrintMetadataTextConfig& cfg)
{
    const Metadata metadata {cfg.path()};

    // not appending any newline to print the exact text
    std::cout << metadata.text();
    std::cout.flush();
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
        printMetadataText(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const InspectConfig *>(cfg.get())) {
        return startInteractive(*specCfg);
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
