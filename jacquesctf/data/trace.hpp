/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TRACE_HPP
#define _JACQUES_DATA_TRACE_HPP

#include <memory>
#include <vector>
#include <boost/core/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include "aliases.hpp"
#include "utils.hpp"
#include "data/data-stream-file.hpp"
#include "data/metadata.hpp"

namespace jacques {

class Trace :
    boost::noncopyable
{
public:
    using DataStreamFiles = std::vector<std::unique_ptr<DataStreamFile>>;

public:
    explicit Trace(const std::vector<boost::filesystem::path>& dataStreamFilePaths);

public:
    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

    const DataStreamFiles& dataStreamFiles() const noexcept
    {
        return _dataStreamFiles;
    }

private:
    void _createMetadata(const boost::filesystem::path& path);

private:
    DataStreamFiles _dataStreamFiles;
    std::unique_ptr<Metadata> _metadata;
};

} // namespace jacques

#endif // _JACQUES_DATA_TRACE_HPP
