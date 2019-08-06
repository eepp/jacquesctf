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

#include "config.hpp"
#include "interactive.hpp"
#include "utils.hpp"

namespace jacques {

static void appendPeriod(std::ostream& stream, const char *what)
{
    assert(std::strlen(what) > 0);

    // append period to exception message if there's none
    if (what[std::strlen(what) - 1] != '.') {
        stream << '.';
    }
}

// creates a Jacques CTF configuration from command-line arguments
static std::unique_ptr<const Config> createConfig(const int argc,
                                                  const char *argv[])
{
    try {
        return configFromArgs(argc, argv);
    } catch (const CliError& ex) {
        utils::error() << "Command-line error: " << ex.what();
        appendPeriod(std::cerr, ex.what());
        std::cerr << std::endl;
        return nullptr;
    } catch (const std::exception& ex) {
        utils::error() << ex.what();
        appendPeriod(std::cerr, ex.what());
        std::cerr << std::endl;
        return nullptr;
    }
}

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

static bool printMetadataText(const PrintMetadataTextConfig& cfg)
{
    std::unique_ptr<const yactfr::MetadataStream> stream;

    try {
        std::ifstream fileStream {cfg.path().string().c_str(),
                                  std::ios::in | std::ios::binary};
        stream = yactfr::createMetadataStream(fileStream);
    } catch (const yactfr::InvalidMetadataStream& ex) {
        utils::error() << "Invalid metadata stream: " << ex.what() << std::endl;
        return false;
    } catch (const yactfr::InvalidMetadata& ex) {
        utils::error() << "Invalid metadata: " << ex.what() << std::endl;
        return false;
    } catch (const yactfr::MetadataParseError& ex) {
        utils::printMetadataParseError(std::cerr, cfg.path().string(), ex);
        return false;
    }

    assert(stream);

    // not appending any newline to print the exact text
    std::cout << stream->text();
    return true;
}

static bool jacques(const int argc, const char *argv[])
{

    auto cfg = createConfig(argc, argv);

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
    return jacques::jacques(argc, argv) ? 0 : 1;
}
