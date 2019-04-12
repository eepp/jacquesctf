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
#include <cassert>

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

void Config::_expandDir(std::list<boost::filesystem::path>& tmpFilePaths,
                        const boost::filesystem::path& path)
{
    namespace bfs = boost::filesystem;

    assert(bfs::is_directory(path));

    const bool hasMetadata = bfs::exists(path / "metadata");
    std::vector<bfs::path> thisDirStreamFilePaths;

    for (const auto& entry : boost::make_iterator_range(bfs::directory_iterator(path), {})) {
        const auto& entryPath = entry.path();

        if (!entryPath.has_filename()) {
            continue;
        }

        if (entryPath.filename() == "metadata") {
            // skip `metadata` file
            continue;
        }

        if (bfs::is_directory(entryPath)) {
            // expand subdirectory
            this->_expandDir(tmpFilePaths, entryPath);
            continue;
        }

        if (!entryPath.filename().string().empty() &&
                entryPath.filename().string()[0] == '.') {
            // hidden file
            continue;
        }

        if (hasMetadata) {
            // it's a CTF directory!
            thisDirStreamFilePaths.push_back(entryPath);
        }
    }

    std::sort(std::begin(thisDirStreamFilePaths),
              std::end(thisDirStreamFilePaths));
    tmpFilePaths.insert(std::end(tmpFilePaths),
                        std::begin(thisDirStreamFilePaths),
                        std::end(thisDirStreamFilePaths));
}

void Config::_expandPaths(const std::vector<boost::filesystem::path>& origFilePaths)
{
    namespace bfs = boost::filesystem;

    std::list<bfs::path> tmpFilePaths;

    for (const auto& path : origFilePaths) {
        if (!bfs::exists(path)) {
            std::ostringstream ss;

            ss << "File or directory `" << path.string() << "` does not exist.";
            throw CliError {ss.str()};
        }

        if (!bfs::is_directory(path) && path.filename() == "metadata") {
            if (origFilePaths.size() > 1) {
                throw CliError {"Can only specify a single CTF metadata file."};
            }

            _filePaths.push_back(path);
            _cmd = Command::PRINT_METADATA_TEXT;
            return;
        }

        if (bfs::is_directory(path)) {
            this->_expandDir(tmpFilePaths, path);
        } else {
            const auto metadataPath = path.parent_path() / "metadata";

            if (!bfs::exists(metadataPath)) {
                std::ostringstream ss;

                ss << "File `" << path.string() <<
                      "` is not part of a CTF trace (missing `metadata` file in its directory).";
                throw CliError {ss.str()};
            }

            tmpFilePaths.push_back(path);
        }
    }

    if (tmpFilePaths.empty()) {
        throw CliError {"No file paths to use."};
    }

    std::unordered_set<std::string> pathSet;
    auto it = std::begin(tmpFilePaths);

    while (it != std::end(tmpFilePaths)) {
        auto nextIt = it;

        ++nextIt;

        if (pathSet.count(it->string()) > 0) {
            // remove duplicate
            tmpFilePaths.erase(it);
        } else {
            pathSet.insert(it->string());
        }

        it = nextIt;
    }

    _filePaths = std::move(tmpFilePaths);
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
        throw CliError {"Missing trace, data stream file, or metadata stream file path."};
    }

    const auto pathArgs = vm["paths"].as<std::vector<std::string>>();
    std::vector<boost::filesystem::path> origFilePaths;

    std::copy(std::begin(pathArgs), std::end(pathArgs),
              std::back_inserter(origFilePaths));
    this->_expandPaths(origFilePaths);
}

} // namespace jacques
