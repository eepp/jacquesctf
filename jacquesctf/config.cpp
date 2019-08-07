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

SinglePathConfig::SinglePathConfig(const bfs::path& path) :
    _path {path}
{
}

InspectConfig::InspectConfig(std::vector<bfs::path>&& paths) :
    _paths {std::move(paths)}
{
}

PrintMetadataTextConfig::PrintMetadataTextConfig(const bfs::path& path) :
    SinglePathConfig {path}
{
}

ListPacketsConfig::ListPacketsConfig(const bfs::path& path,
                                     Format format, bool withHeader) :
    SinglePathConfig {path},
    _format {format},
    _withHeader {withHeader}
{
}

CopyPacketsConfig::CopyPacketsConfig(const bfs::path& srcPath,
                                     const std::string& packetIndexes,
                                     const bfs::path& dstPath) :
    _srcPath {srcPath},
    _packetIndexes {packetIndexes},
    _dstPath {dstPath}
{
}

CreateLttngIndexConfig::CreateLttngIndexConfig(std::vector<bfs::path>&& paths) :
    _paths {std::move(paths)}
{
}

PrintCliUsageConfig::PrintCliUsageConfig()
{
}

PrintVersionConfig::PrintVersionConfig()
{
}

static void expandDir(std::list<bfs::path>& tmpFilePaths, const bfs::path& path)
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

static std::vector<bfs::path> expandPaths(const std::vector<bfs::path>& origFilePaths,
                                          const bool allowMetadata = true)
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
            if (allowMetadata) {
                if (origFilePaths.size() > 1) {
                    throw CliError {"Can only specify a single CTF metadata file."};
                }

                return {path};
            }

            throw CliError {"Cannot specify CTF metadata file."};
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

static std::vector<bfs::path> getExpandedPaths(const std::vector<std::string>& args)
{
    if (args.empty()) {
        throw CliError {"Missing trace directory path, data stream file path, or metadata stream file path."};
    }

    std::vector<bfs::path> origFilePaths;

    std::copy(std::begin(args), std::end(args),
              std::back_inserter(origFilePaths));

    return expandPaths(origFilePaths);
}

static std::unique_ptr<const Config> inspectConfigFromArgs(const std::vector<std::string>& args)
{
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

    auto expandedPaths = getExpandedPaths(vm["paths"].as<std::vector<std::string>>());

    if (expandedPaths.size() == 1 && expandedPaths.front().filename() == "metadata") {
        return std::make_unique<PrintMetadataTextConfig>(expandedPaths.front());
    }

    return std::make_unique<InspectConfig>(std::move(expandedPaths));
}

static std::unique_ptr<const Config> createLttngIndexConfigFromArgs(const std::vector<std::string>& args)
{
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

    auto expandedPaths = getExpandedPaths(vm["paths"].as<std::vector<std::string>>());

    return std::make_unique<CreateLttngIndexConfig>(std::move(expandedPaths));
}

static void checkLooksLikeDataStreamFile(const bfs::path& path)
{
    if (!bfs::is_regular_file(path)) {
        std::ostringstream ss;

        ss << "`" << path.string() << "` is not a regular file.";
        throw CliError {ss.str()};
    }

    if (path.filename() == "metadata") {
        std::ostringstream ss;

        ss << "Cannot list the packets of a metadata file: `" <<
              path.string() << "`.";
        throw CliError {ss.str()};
    }
}

static std::unique_ptr<const Config> listPacketsConfigFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDesc {""};

    optDesc.add_options()
        ("machine,m", "")
        ("header", "")
        ("path", bpo::value<std::string>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("path", 1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDesc).
                   positional(posDesc).run(), vm);
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        std::abort();
    }

    if (vm.count("machine") == 0) {
        throw CliError {
            "Missing --machine option (required by this version)."
        };
    }

    if (vm.count("path") == 0) {
        throw CliError {"Missing data stream file path."};
    }

    const auto path = bfs::path {vm["path"].as<std::string>()};

    checkLooksLikeDataStreamFile(path);
    return std::make_unique<ListPacketsConfig>(path,
                                               ListPacketsConfig::Format::MACHINE,
                                               vm.count("header") == 1);
}

static std::unique_ptr<const Config> copyPacketsConfigFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDesc {""};

    optDesc.add_options()
        ("src-path", bpo::value<std::string>(), "")
        ("packet-indexes", bpo::value<std::string>(), "")
        ("dst-path", bpo::value<std::string>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("src-path", 1)
           .add("packet-indexes", 1)
           .add("dst-path", 1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDesc).
                   positional(posDesc).run(), vm);
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        std::abort();
    }

    if (vm.count("src-path") == 0) {
        throw CliError {"Missing source data stream file path."};
    }

    if (vm.count("packet-indexes") == 0) {
        throw CliError {"Missing packet indexes."};
    }

    if (vm.count("dst-path") == 0) {
        throw CliError {"Missing destination data stream file path."};
    }

    const auto srcPath = bfs::path {vm["src-path"].as<std::string>()};
    const auto dstPath = bfs::path {vm["dst-path"].as<std::string>()};

    checkLooksLikeDataStreamFile(srcPath);

    if (bfs::exists(dstPath)) {
        checkLooksLikeDataStreamFile(dstPath);
    }

    if (srcPath == dstPath) {
        std::ostringstream ss;

        ss << "Source and destination data stream files are the same: `" <<
              srcPath.string() << "`.";
        throw CliError {ss.str()};
    }

    return std::make_unique<CopyPacketsConfig>(srcPath,
                                               vm["packet-indexes"].as<std::string>(),
                                               dstPath);
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

        if (vm.count("help") > 0) {
            return std::make_unique<PrintCliUsageConfig>();
        }

        if (vm.count("version") > 0) {
            return std::make_unique<PrintVersionConfig>();
        }

        if (vm.count("args") == 0) {
            throw CliError {"Missing command name, trace directory path, data stream file path, or metadata stream file path."};
        }

        const auto args = vm["args"].as<std::vector<std::string>>();

        assert(!args.empty());

        bool removeCmdName = false;
        const static std::string listPacketsCmdName {"list-packets"};
        const static std::string copyPacketsCmdName {"copy-packets"};
        const static std::string createLttngIndexCmdName {"create-lttng-index"};

        if (args[0] == "inspect" || args[0] == listPacketsCmdName ||
                args[0] == copyPacketsCmdName ||
                args[0] == createLttngIndexCmdName) {
            removeCmdName = true;
        }

        std::vector<std::string> extraArgs = bpo::collect_unrecognized(parsedOpts.options,
                                                                       bpo::include_positional);

        if (removeCmdName) {
            extraArgs.erase(extraArgs.begin());
        }

        if (args[0] == listPacketsCmdName) {
            return listPacketsConfigFromArgs(extraArgs);
        } else if (args[0] == copyPacketsCmdName) {
            return copyPacketsConfigFromArgs(extraArgs);
        } else if (args[0] == createLttngIndexCmdName) {
            return createLttngIndexConfigFromArgs(extraArgs);
        }

        // `inspect` command is the default
        return inspectConfigFromArgs(extraArgs);
    } catch (const bpo::error& ex) {
        throw CliError {ex.what()};
    } catch (...) {
        throw;
    }
}

} // namespace jacques
