/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "trace.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

Trace::Trace(const std::vector<bfs::path>& dsFilePaths)
{
    assert(!dsFilePaths.empty());

    const auto metadataPath = dsFilePaths.front().parent_path() / "metadata";

    assert(bfs::is_regular_file(metadataPath));
    _metadata = std::make_unique<Metadata>(metadataPath);

    for (const auto& dsfPath : dsFilePaths) {
        _dsFiles.push_back(std::make_unique<DsFile>(dsfPath, *_metadata));
    }
}

} // namespace jacques
