/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_METADATA_ERROR_HPP
#define _JACQUES_DATA_METADATA_ERROR_HPP

#include <stdexcept>
#include <utility>
#include <boost/filesystem.hpp>

namespace jacques {

template <typename SubErrorT>
class MetadataError final :
    public std::runtime_error
{
public:
    explicit MetadataError(boost::filesystem::path path, const SubErrorT& subError) :
        std::runtime_error {subError.what()},
        _path {std::move(path)},
        _subError {subError}
    {
    }

    boost::filesystem::path path() const noexcept
    {
        return _path;
    }

    const SubErrorT& subError() const noexcept
    {
        return _subError;
    }

private:
    const boost::filesystem::path _path;
    const SubErrorT _subError;
};

} // namespace jacques

#endif // _JACQUES_DATA_METADATA_ERROR_HPP
