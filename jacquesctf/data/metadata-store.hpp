/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_METADATA_STORE_HPP
#define _JACQUES_METADATA_STORE_HPP

#include <memory>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include "metadata.hpp"

namespace jacques {

class MetadataStore
{
public:
    MetadataStore();
    std::shared_ptr<const Metadata> metadata(const boost::filesystem::path& path);

private:
    std::unordered_map<std::string, std::shared_ptr<const Metadata>> _store;
};

} // namespace jacques

#endif // _JACQUES_METADATA_STORE_HPP
