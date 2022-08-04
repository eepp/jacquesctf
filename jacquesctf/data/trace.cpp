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

Trace::Trace(const bfs::path& metadataPath, const std::vector<bfs::path>& dsFilePaths)
{
    this->_createMetadata(metadataPath);

    for (auto& dsfPath : dsFilePaths) {
        _dsFiles.emplace_back(new DsFile {*this, dsfPath});
    }
}

Trace::Trace(const std::vector<bfs::path>& dsFilePaths) :
    Trace {dsFilePaths.front().parent_path() / "metadata", dsFilePaths}
{
}

void Trace::_createMetadata(const bfs::path& metadataPath)
{
    assert(bfs::is_regular_file(metadataPath));

    std::ifstream metadataFileStream {
        metadataPath.string().c_str(), std::ios::in | std::ios::binary
    };

    try {
        auto metadataStream = yactfr::createMetadataStream(metadataFileStream);
        auto traceTypeMetadataStreamUuidPair = yactfr::fromMetadataText(metadataStream->text());

        _metadata = std::make_unique<Metadata>(metadataPath,
                                               std::move(traceTypeMetadataStreamUuidPair.first),
                                               std::move(metadataStream),
                                               std::move(traceTypeMetadataStreamUuidPair.second));
    } catch (const yactfr::InvalidMetadataStream& exc) {
        throw MetadataError<yactfr::InvalidMetadataStream> {metadataPath, exc};
    } catch (const yactfr::TextParseError& exc) {
        throw MetadataError<yactfr::TextParseError> {metadataPath, exc};
    }
}

std::unique_ptr<Trace> Trace::withoutDsFiles(const bfs::path& traceDir)
{
    return std::make_unique<Trace>(traceDir / "metadata");
}

} // namespace jacques
