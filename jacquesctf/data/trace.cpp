/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <fstream>
#include <yactfr/yactfr.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include "trace.hpp"
#include "ds-file.hpp"
#include "utils.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

TraceEnvStreamError::TraceEnvStreamError(std::string msg, boost::optional<bfs::path> path,
                                         boost::optional<Index> offset) :
    _msg {std::move(msg)},
    _path {std::move(path)},
    _offset {std::move(offset)}
{
}

Trace::Trace(const bfs::path& metadataPath, const std::vector<bfs::path>& dsFilePaths,
             const bool readEnvStream)
{
    this->_createMetadataAndEnv(metadataPath, readEnvStream);

    for (auto& dsfPath : dsFilePaths) {
        _dsFiles.emplace_back(new DsFile {*this, dsfPath});
    }
}

Trace::Trace(const std::vector<bfs::path>& dsFilePaths, const bool readEnvStream) :
    Trace {dsFilePaths.front().parent_path() / "metadata", dsFilePaths, readEnvStream}
{
}

void Trace::_createMetadataAndEnv(const bfs::path& metadataPath, const bool readEnvStream)
{
    assert(bfs::is_regular_file(metadataPath));

    std::ifstream metadataFileStream {
        metadataPath.string().c_str(), std::ios::in | std::ios::binary
    };

    try {
        auto metadataStream = yactfr::createMetadataStream(metadataFileStream);
        auto traceTypeEnvPair = yactfr::fromMetadataText(metadataStream->text());

        _metadata = std::make_unique<Metadata>(metadataPath, std::move(traceTypeEnvPair.first),
                                               std::move(metadataStream));

        if (_metadata->traceType().majorVersion() == 1) {
            _env = std::move(traceTypeEnvPair.second);
        }
    } catch (const yactfr::InvalidMetadataStream& exc) {
        throw MetadataError<yactfr::InvalidMetadataStream> {metadataPath, exc};
    } catch (const yactfr::TextParseError& exc) {
        throw MetadataError<yactfr::TextParseError> {metadataPath, exc};
    }

    if (_metadata->traceType().majorVersion() == 2 && readEnvStream) {
        this->_readEnvStream();
    }
}

void Trace::_readEnvStream()
{
    /*
     * Here we need to open each potential data stream file within the
     * immediate directory of this trace, and then, for each one, check
     * if it's a trace environment data stream file thanks to the
     * namespace and name properties of its data stream type.
     */
    const auto traceDir = _metadata->path().parent_path();
    std::set<bfs::path> dsFilePaths;

    for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(traceDir), {})) {
        auto& entryPath = entry.path();

        try {
            if (this->_looksLikeTraceEnvStream(entryPath)) {
                dsFilePaths.insert(entryPath);
            }
        } catch (const yactfr::DecodingError& exc) {
            _envStreamError = TraceEnvStreamError {exc.what(), entryPath, exc.offset()};
            return;
        } catch (const yactfr::InvalidTraceEnvironmentStream& exc) {
            _envStreamError = TraceEnvStreamError {exc.what(), entryPath, exc.offset()};
            return;
        }
    }

    if (dsFilePaths.empty()) {
        return;
    } else if (dsFilePaths.size() == 1) {
        _envStreamPath = *dsFilePaths.begin();

        try {
            yactfr::MemoryMappedFileViewFactory factory {_envStreamPath->string(), 4 << 20};
            yactfr::TraceEnvironmentStreamDecoder decoder {_metadata->traceType(), factory};

            _env = decoder.decode();
        } catch (const yactfr::DecodingError& exc) {
            _envStreamError = TraceEnvStreamError {exc.what(), *_envStreamPath, exc.offset()};
            return;
        } catch (const yactfr::InvalidTraceEnvironmentStream& exc) {
            _envStreamError = TraceEnvStreamError {exc.what(), *_envStreamPath, exc.offset()};
            return;
        }
    } else if (dsFilePaths.size() > 1) {
        std::vector<std::string> dsFilePathStrs;

        std::transform(dsFilePaths.cbegin(), dsFilePaths.cend(),
                       std::back_inserter(dsFilePathStrs), [](auto& path) {
            return path.string();
        });

        std::ostringstream ss;

        ss << "The trace contains more than one trace environment stream " <<
              "(expecting a single one); candidates are " <<
              utils::csvListStr(dsFilePathStrs) << '.';
        _envStreamError = TraceEnvStreamError {ss.str()};
        return;
    }
}

bool Trace::_looksLikeTraceEnvStream(const bfs::path& path) const
{
    if (!utils::looksLikeDsFilePath(path)) {
        return false;
    }

    yactfr::MemoryMappedFileViewFactory factory {path.string(), 4 << 20};
    yactfr::TraceEnvironmentStreamDecoder decoder {_metadata->traceType(), factory};

    return decoder.isTraceEnvironmentStream();
}

std::unique_ptr<Trace> Trace::withoutDsFiles(const bfs::path& traceDir)
{
    return std::make_unique<Trace>(traceDir / "metadata");
}

} // namespace jacques
