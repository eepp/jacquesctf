/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <memory>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include "metadata-store.hpp"
#include "metadata.hpp"

namespace jacques {

MetadataStore::MetadataStore()
{
}

std::shared_ptr<const Metadata> MetadataStore::metadata(const boost::filesystem::path& path)
{
    namespace bfs = boost::filesystem;

    // canonical path should be unique enough here
    assert(bfs::exists(path));

    const auto canPathStr = bfs::canonical(path).string();
    const auto it = _store.find(canPathStr);

    if (it != std::end(_store)) {
        return it->second;
    }

    auto metadataSp = std::make_shared<const Metadata>(path);

    _store[canPathStr] = metadataSp;
    return metadataSp;
}

} // namespace jacques
