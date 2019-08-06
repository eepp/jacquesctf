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

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;

namespace jacques {

CliError::CliError(const std::string& msg) :
    std::runtime_error {msg}
{
}

Config::Config()
{
}

Config::~Config()
{
}

InspectConfig::InspectConfig(std::vector<bfs::path>&& paths) :
    _paths {std::move(paths)}
{
}

PrintMetadataTextConfig::PrintMetadataTextConfig(const bfs::path& path) :
    _path {path}
{
}

PrintCliUsageConfig::PrintCliUsageConfig()
{
}

PrintVersionConfig::PrintVersionConfig()
{
}

static
void expandDir(std::list<bfs::path>& tmpFilePaths, const bfs::path& path)
{
    namespace bfs = bfs;

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
            expandDir(tmpFilePaths, entryPath);
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

static
std::vector<bfs::path> expandPaths(const std::vector<bfs::path>& origFilePaths)
{
    namespace bfs = bfs;

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

            return {path};
        }

        if (bfs::is_directory(path)) {
            expandDir(tmpFilePaths, path);
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

    std::vector<bfs::path> expandedPaths;

    std::copy(std::begin(tmpFilePaths), std::end(tmpFilePaths),
              std::back_inserter(expandedPaths));
    return expandedPaths;
}

std::unique_ptr<const Config> inspectConfigFromArgs(const std::vector<std::string>& args)
{
    if (args.empty()) {
        throw CliError {"Missing trace directory path, data stream file path, or metadata stream file path."};
    }

    bpo::options_description optDesc {""};

    optDesc.add_options()
        ("paths", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("paths", -1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDesc).
                   positional(posDesc).run(), vm);
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        std::abort();
    }

    const auto paths = vm["paths"].as<std::vector<std::string>>();

    std::vector<bfs::path> origFilePaths;

    std::copy(std::begin(paths), std::end(paths),
              std::back_inserter(origFilePaths));

    auto expandedPaths = expandPaths(origFilePaths);

    if (expandedPaths.size() == 1 && expandedPaths.front().filename() == "metadata") {
        return std::make_unique<PrintMetadataTextConfig>(expandedPaths.front());
    }

    return std::make_unique<InspectConfig>(std::move(expandedPaths));
}

std::unique_ptr<const Config> configFromArgs(const int argc,
                                             const char *argv[])
{
    bpo::options_description optDesc {""};

    optDesc.add_options()
        ("help,h", "")
        ("version,V", "")
        ("args", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("args", -1);

    bpo::variables_map vm;

    try {
        const auto parsedOpts = bpo::command_line_parser(argc, argv).options(optDesc).
                                positional(posDesc).allow_unregistered().run();

        bpo::store(parsedOpts, vm);

        if (vm.count("help")) {
            return std::make_unique<PrintCliUsageConfig>();
        }

        if (vm.count("version")) {
            return std::make_unique<PrintVersionConfig>();
        }

        if (!vm.count("args")) {
            throw CliError {"Missing command name, trace directory path, data stream file path, or metadata stream file path."};
        }

        const auto args = vm["args"].as<std::vector<std::string>>();

        assert(!args.empty());

        bool removeCmdName = true;

        if (args[0] == "inspect") {
        } else {
            removeCmdName = false;
        }

        std::vector<std::string> extraArgs = bpo::collect_unrecognized(parsedOpts.options,
                                                                       bpo::include_positional);

        if (removeCmdName) {
            extraArgs.erase(extraArgs.begin());
        }

        return inspectConfigFromArgs(extraArgs);
        throw;
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        throw;
    }
}

} // namespace jacques
