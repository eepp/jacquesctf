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
#include "ds-file.hpp"
#include "metadata.hpp"

namespace jacques {

class Trace final :
    boost::noncopyable
{
public:
    using DsFiles = std::vector<std::unique_ptr<DsFile>>;

public:
    explicit Trace(const std::vector<boost::filesystem::path>& dsFilePaths);

public:
    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

    const DsFiles& dsFiles() const noexcept
    {
        return _dsFiles;
    }

private:
    void _createMetadata(const boost::filesystem::path& path);

private:
    DsFiles _dsFiles;
    std::unique_ptr<Metadata> _metadata;
};

} // namespace jacques

#endif // _JACQUES_DATA_TRACE_HPP
