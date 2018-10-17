/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_METADATE_ERROR_HPP
#define _JACQUES_METADATE_ERROR_HPP

#include <stdexcept>

#include "metadata.hpp"

namespace jacques {

class MetadataError :
    public std::runtime_error
{
public:
    explicit MetadataError(std::shared_ptr<const Metadata> metadata) :
        std::runtime_error {"Metadata error"},
        _metadata {metadata}
    {
    }

    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

private:
    std::shared_ptr<const Metadata> _metadata;
};

} // namespace jacques

#endif // _JACQUES_METADATE_ERROR_HPP
