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

#include "cfg.hpp"
#include "utils.hpp"

namespace jacques {

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;

CliError::CliError(const std::string& msg) :
    std::runtime_error {msg}
{
}

SinglePathCfg::SinglePathCfg(bfs::path&& path) :
    _path {std::move(path)}
{
}

InspectCfg::InspectCfg(std::vector<bfs::path> paths) :
    _paths {std::move(paths)}
{
}

PrintMetadataTextCfg::PrintMetadataTextCfg(bfs::path path) :
    SinglePathCfg {std::move(path)}
{
}

ListPktsCfg::ListPktsCfg(bfs::path path, Fmt fmt, bool withHeader) :
    SinglePathCfg {std::move(path)},
    _fmt {fmt},
    _withHeader {withHeader}
{
}

CopyPktsCfg::CopyPktsCfg(bfs::path srcPath, std::string pktIndexes, bfs::path dstPath) :
    _srcPath {std::move(srcPath)},
    _pktIndexes {std::move(pktIndexes)},
    _dstPath {std::move(dstPath)}
{
}

CreateLttngIndexCfg::CreateLttngIndexCfg(std::vector<bfs::path> paths) :
    _paths {std::move(paths)}
{
}

namespace {

void expandDir(std::list<bfs::path>& tmpFilePaths, const bfs::path& path)
{
    namespace bfs = bfs;

    assert(bfs::is_directory(path));

    const bool hasMetadata = bfs::exists(path / "metadata");
    std::vector<bfs::path> thisDirDsFilePaths;

    for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(path), {})) {
        auto& entryPath = entry.path();

        if (utils::looksLikeDsFilePath(entryPath) && hasMetadata) {
            thisDirDsFilePaths.push_back(entryPath);
        } else if (bfs::is_directory(entryPath) && !utils::isHiddenFile(entryPath)) {
            // expand subdirectory
            expandDir(tmpFilePaths, entryPath);
        }
    }

    std::sort(thisDirDsFilePaths.begin(), thisDirDsFilePaths.end());
    tmpFilePaths.insert(tmpFilePaths.end(), thisDirDsFilePaths.begin(),
                        thisDirDsFilePaths.end());
}

std::vector<bfs::path> expandPaths(const std::vector<bfs::path>& origFilePaths,
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
    auto it = tmpFilePaths.begin();

    while (it != tmpFilePaths.end()) {
        const auto nextIt = std::next(it);

        if (pathSet.count(it->string()) > 0) {
            // remove duplicate
            tmpFilePaths.erase(it);
        } else {
            pathSet.insert(it->string());
        }

        it = nextIt;
    }

    std::vector<bfs::path> expandedPaths;

    std::copy(tmpFilePaths.begin(), tmpFilePaths.end(), std::back_inserter(expandedPaths));
    return expandedPaths;
}

std::vector<bfs::path> getExpandedPaths(const std::vector<std::string>& args)
{
    if (args.empty()) {
        throw CliError {"Missing trace directory path, data stream file path, or metadata stream file path."};
    }

    std::vector<bfs::path> origFilePaths;

    std::copy(args.begin(), args.end(), std::back_inserter(origFilePaths));
    return expandPaths(origFilePaths);
}

std::unique_ptr<const Cfg> inspectCfgFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDescr {""};

    optDescr.add_options()
        ("paths", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("paths", -1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDescr).positional(posDesc).run(), vm);
    } catch (const bpo::error& exc) {
        throw CliError {exc.what()};
    } catch (...) {
        std::abort();
    }

    auto expandedPaths = getExpandedPaths(vm["paths"].as<std::vector<std::string>>());

    if (expandedPaths.size() == 1 && expandedPaths.front().filename() == "metadata") {
        return std::make_unique<PrintMetadataTextCfg>(std::move(expandedPaths.front()));
    }

    return std::make_unique<InspectCfg>(std::move(expandedPaths));
}

std::unique_ptr<const Cfg> createLttngIndexCfgFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDescr {""};

    optDescr.add_options()
        ("paths", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("paths", -1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDescr).positional(posDesc).run(), vm);
    } catch (const bpo::error& exc) {
        throw CliError {exc.what()};
    } catch (...) {
        std::abort();
    }

    auto expandedPaths = getExpandedPaths(vm["paths"].as<std::vector<std::string>>());

    return std::make_unique<CreateLttngIndexCfg>(std::move(expandedPaths));
}

void checkLooksLikeDsFile(const bfs::path& path)
{
    if (!bfs::is_regular_file(path)) {
        std::ostringstream ss;

        ss << "`" << path.string() << "` is not a regular file.";
        throw CliError {ss.str()};
    }

    if (path.filename() == "metadata") {
        std::ostringstream ss;

        ss << "Cannot list the packets of a metadata file: `" << path.string() << "`.";
        throw CliError {ss.str()};
    }
}

std::unique_ptr<const Cfg> listPktsCfgFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDescr {""};

    optDescr.add_options()
        ("machine,m", "")
        ("header", "")
        ("path", bpo::value<std::string>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("path", 1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDescr). positional(posDesc).run(), vm);
    } catch (const bpo::error& exc) {
        throw CliError {exc.what()};
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

    auto path = bfs::path {vm["path"].as<std::string>()};

    checkLooksLikeDsFile(path);
    return std::make_unique<ListPktsCfg>(std::move(path), ListPktsCfg::Fmt::MACHINE,
                                         vm.count("header") == 1);
}

std::unique_ptr<const Cfg> copyPktsCfgFromArgs(const std::vector<std::string>& args)
{
    bpo::options_description optDescr {""};

    optDescr.add_options()
        ("src-path", bpo::value<std::string>(), "")
        ("packet-indexes", bpo::value<std::string>(), "")
        ("dst-path", bpo::value<std::string>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("src-path", 1).add("packet-indexes", 1).add("dst-path", 1);

    bpo::variables_map vm;

    try {
        bpo::store(bpo::command_line_parser(args).options(optDescr).positional(posDesc).run(), vm);
    } catch (const bpo::error& exc) {
        throw CliError {exc.what()};
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

    auto srcPath = bfs::path {vm["src-path"].as<std::string>()};
    auto dstPath = bfs::path {vm["dst-path"].as<std::string>()};

    checkLooksLikeDsFile(srcPath);

    if (bfs::exists(dstPath)) {
        checkLooksLikeDsFile(dstPath);
    }

    if (srcPath == dstPath) {
        std::ostringstream ss;

        ss << "Source and destination data stream files are the same: `" <<
              srcPath.string() << "`.";
        throw CliError {ss.str()};
    }

    return std::make_unique<CopyPktsCfg>(std::move(srcPath),
                                         vm["packet-indexes"].as<std::string>(),
                                         std::move(dstPath));
}

} // namespace

std::unique_ptr<const Cfg> cfgFromArgs(const int argc, const char *argv[])
{
    bpo::options_description optDescr {""};

    optDescr.add_options()
        ("help,h", "")
        ("version,V", "")
        ("args", bpo::value<std::vector<std::string>>(), "");

    bpo::positional_options_description posDesc;

    posDesc.add("args", -1);

    bpo::variables_map vm;

    try {
        const auto parsedOpts = bpo::command_line_parser(argc, argv).options(optDescr).
                                positional(posDesc).allow_unregistered().run();

        bpo::store(parsedOpts, vm);

        if (vm.count("help") > 0) {
            return std::make_unique<PrintCliUsageCfg>();
        }

        if (vm.count("version") > 0) {
            return std::make_unique<PrintVersionCfg>();
        }

        if (vm.count("args") == 0) {
            throw CliError {"Missing command name, trace directory path, data stream file path, or metadata stream file path."};
        }

        const auto args = vm["args"].as<std::vector<std::string>>();

        assert(!args.empty());

        auto removeCmdName = false;
        constexpr const char *listPktsCmdName = "list-packets";
        constexpr const char *copyPktsCmdName = "copy-packets";
        constexpr const char *createLttngIndexCmdName = "create-lttng-index";

        if (args[0] == "inspect" || args[0] == listPktsCmdName ||
                args[0] == copyPktsCmdName || args[0] == createLttngIndexCmdName) {
            removeCmdName = true;
        }

        std::vector<std::string> extraArgs = bpo::collect_unrecognized(parsedOpts.options,
                                                                       bpo::include_positional);

        if (removeCmdName) {
            extraArgs.erase(extraArgs.begin());
        }

        if (args[0] == listPktsCmdName) {
            return listPktsCfgFromArgs(extraArgs);
        } else if (args[0] == copyPktsCmdName) {
            return copyPktsCfgFromArgs(extraArgs);
        } else if (args[0] == createLttngIndexCmdName) {
            return createLttngIndexCfgFromArgs(extraArgs);
        }

        // `inspect` command is the default
        return inspectCfgFromArgs(extraArgs);
    } catch (const bpo::error& exc) {
        throw CliError {exc.what()};
    } catch (...) {
        throw;
    }
}

} // namespace jacques
