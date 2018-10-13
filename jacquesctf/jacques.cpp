/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
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
        return std::make_unique<const Config>(argc, argv);
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
    std::cout << "Usage: " << cmdName << " [OPTIONS] PATH..." << std::endl <<
                 std::endl <<
                 "Options:" << std::endl <<
                 std::endl <<
                 "  --bytes-per-row-bin=COUNT, -b COUNT    Show COUNT bytes/row in binary mode" << std::endl <<
                 "  --bytes-per-row-hex=COUNT, -x COUNT    Show COUNT bytes/row in hex mode" << std::endl <<
                 "  --help, -h                             Print usage and exit" << std::endl <<
                 "  --version, -V                          Print version and exit" << std::endl <<
                 std::endl <<
                 "If PATH is a CTF metadata file, print its text content and exit." << std::endl <<
                 "If PATH is a CTF data stream file, inspect this file." << std::endl <<
                 "If PATH is a CTF trace directory, inspect all its data stream files." << std::endl;
}

static void printVersion()
{
    std::cout << "Jacques CTF " JACQUES_VERSION << std::endl;
}

static bool printMetadataText(const Config& cfg)
{
    std::unique_ptr<const yactfr::MetadataStream> stream;

    try {
        std::ifstream fileStream {std::begin(cfg.filePaths())->string().c_str(),
                                  std::ios::in | std::ios::binary};
        stream = yactfr::createMetadataStream(fileStream);
    } catch (const yactfr::InvalidMetadataStream& ex) {
        utils::error() << "Invalid metadata stream: " << ex.what() << std::endl;
        return false;
    } catch (const yactfr::InvalidMetadata& ex) {
        utils::error() << "Invalid metadata: " << ex.what() << std::endl;
        return false;
    } catch (const yactfr::MetadataParseError& ex) {
        utils::error() << "Cannot parse metadata text:" << std::endl <<
                          ex.what();
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

    switch (cfg->command()) {
    case Config::Command::PRINT_CLI_USAGE:
        printCliUsage(argv[0]);
        break;

    case Config::Command::PRINT_VERSION:
        printVersion();
        break;

    case Config::Command::PRINT_METADATA_TEXT:
        printMetadataText(*cfg);
        break;

    case Config::Command::INSPECT_FILES:
        return startInteractive(*cfg);
    }

    return true;
}

} // namespace jacques

int main(const int argc, const char *argv[])
{
    return jacques::jacques(argc, argv) ? 0 : 1;
}
