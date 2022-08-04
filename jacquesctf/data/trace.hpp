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
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include "aliases.hpp"
#include "utils.hpp"
#include "metadata.hpp"

namespace jacques {

class DsFile;

class Trace final :
    boost::noncopyable
{
public:
    using DsFiles = std::vector<std::unique_ptr<DsFile>>;

public:
    explicit Trace(const boost::filesystem::path& metadataPath,
                   const std::vector<boost::filesystem::path>& dsFilePaths = {});

    explicit Trace(const std::vector<boost::filesystem::path>& dsFilePaths);

public:
    static std::unique_ptr<Trace> withoutDsFiles(const boost::filesystem::path& traceDir);

    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

    DsFiles& dsFiles() noexcept
    {
        return _dsFiles;
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
