/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <unordered_set>
#include <iostream>

#include "config.hpp"
#include "utils.hpp"

namespace jacques {

CliError::CliError(const std::string& msg) :
    std::runtime_error {msg}
{
}

Config::Config(const int argc, const char *argv[])
{
    this->_parseArgs(argc, argv);
}

void Config::_expandPaths()
{
    namespace bfs = boost::filesystem;

    auto it = std::begin(_filePaths);

    while (it != std::end(_filePaths)) {
        const auto& path = *it;

        if (!bfs::exists(path)) {
            std::ostringstream ss;

            ss << "File or directory `" << path.string() << "` does not exist.";
            throw CliError {ss.str()};
        }

        if (!bfs::is_directory(path) && path.filename() == "metadata") {
            if (_filePaths.size() > 1) {
                throw CliError {"Can only specify a single CTF metadata file."};
            }

            _cmd = Command::PRINT_METADATA_TEXT;
            return;
        }

        if (bfs::is_directory(path)) {
            bfs::path metadataPath {path};

            metadataPath /= "metadata";

            if (!bfs::exists(metadataPath)) {
                std::ostringstream ss;

                ss << "Directory `" << path.string() <<
                      "` is not a CTF trace (missing `metadata` file).";
                throw CliError {ss.str()};
            }

            std::vector<bfs::path> traceFilePaths;

            for (const auto& entry : boost::make_iterator_range(bfs::directory_iterator(path), {})) {
                const auto& entryPath = entry.path();

                if (!entryPath.has_filename()) {
                    continue;
                }

                if (entryPath.filename() == "metadata") {
                    continue;
                }

                if (bfs::is_directory(entryPath)) {
                    continue;
                }

                if (!entryPath.filename().string().empty() &&
                        entryPath.filename().string()[0] == '.') {
                    // hidden file
                    continue;
                }

                traceFilePaths.push_back(entryPath);
            }

            std::sort(std::begin(traceFilePaths), std::end(traceFilePaths));
            it = _filePaths.erase(it);
            _filePaths.insert(it, std::begin(traceFilePaths),
                              std::end(traceFilePaths));
        } else {
            const auto metadataPath = path.parent_path() / "metadata";

            if (!bfs::exists(metadataPath)) {
                std::ostringstream ss;

                ss << "File `" << path.string() <<
                      "` is not part of a CTF trace (missing `metadata` file in its directory).";
                throw CliError {ss.str()};
            }

            ++it;
        }
    }

    if (_filePaths.empty()) {
        throw CliError {"No file paths to use."};
    }

    std::unordered_set<std::string> pathSet;
    it = std::begin(_filePaths);

    while (it != std::end(_filePaths)) {
        auto nextIt = it;

        ++nextIt;

        if (pathSet.count(it->string()) > 0) {
            // remove duplicate
            _filePaths.erase(it);
        } else {
            pathSet.insert(it->string());
        }

        it = nextIt;
    }
}

void Config::_parseArgs(const int argc, const char *argv[])
{
    namespace bpo = boost::program_options;

    bpo::options_description optDesc {"Options"};

    optDesc.add_options()
        ("help,h", "")
        ("version,V", "")
        ("paths", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("paths", -1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(argc, argv).options(optDesc).
                   positional(posDesc).allow_unregistered().run(), vm);
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        std::abort();
    }

    if (vm.count("help")) {
        _cmd = Command::PRINT_CLI_USAGE;
        return;
    }

    if (vm.count("version")) {
        _cmd = Command::PRINT_VERSION;
        return;
    }

    if (!vm.count("paths")) {
        throw CliError {"Missing trace, data stream, or metadata stream path."};
    }

    const auto pathArgs = vm["paths"].as<std::vector<std::string>>();

    std::copy(std::begin(pathArgs), std::end(pathArgs),
              std::back_inserter(_filePaths));
    this->_expandPaths();
}

} // namespace jacques
