/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "data/trace.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

Trace::Trace(const std::vector<bfs::path>& dataStreamFilePaths)
{
    assert(!dataStreamFilePaths.empty());

    const auto metadataPath = dataStreamFilePaths.front().parent_path() / "metadata";

    assert(bfs::is_regular_file(metadataPath));
    _metadata = std::make_unique<Metadata>(metadataPath);

    for (const auto& dsfPath : dataStreamFilePaths) {
        _dataStreamFiles.push_back(std::make_unique<DataStreamFile>(dsfPath,
                                                                    *_metadata));
    }
}

} // namespace jacques
