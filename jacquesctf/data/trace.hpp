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

class TraceEnvStreamError final
{
public:
    explicit TraceEnvStreamError(std::string msg,
                                 boost::optional<boost::filesystem::path> path = boost::none,
                                 boost::optional<Index> offset = boost::none);

    const std::string& msg() const noexcept
    {
        return _msg;
    }

    const boost::optional<boost::filesystem::path>& path() const noexcept
    {
        return _path;
    }

    const boost::optional<Index>& offset() const noexcept
    {
        return _offset;
    }

private:
    std::string _msg;
    boost::optional<boost::filesystem::path> _path;
    boost::optional<Index> _offset;
};

class Trace final :
    boost::noncopyable
{
public:
    using DsFiles = std::vector<std::unique_ptr<DsFile>>;

public:
    explicit Trace(const boost::filesystem::path& metadataPath,
                   const std::vector<boost::filesystem::path>& dsFilePaths = {},
                   bool readEnvStream = true);

    explicit Trace(const std::vector<boost::filesystem::path>& dsFilePaths,
                   bool readEnvStream = true);

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

    const boost::optional<boost::filesystem::path>& envStreamPath() const noexcept
    {
        return _envStreamPath;
    }

    const boost::optional<yactfr::TraceEnvironment>& env() const noexcept
    {
        return _env;
    }

    const boost::optional<TraceEnvStreamError>& envStreamError() const noexcept
    {
        return _envStreamError;
    }

private:
    void _createMetadataAndEnv(const boost::filesystem::path& path, bool readEnvStream);
    void _readEnvStream();
    bool _looksLikeTraceEnvStream(const boost::filesystem::path& path) const;

private:
    DsFiles _dsFiles;
    boost::optional<boost::filesystem::path> _envStreamPath;
    boost::optional<yactfr::TraceEnvironment> _env;
    boost::optional<TraceEnvStreamError> _envStreamError;
    std::unique_ptr<Metadata> _metadata;
};

} // namespace jacques

#endif // _JACQUES_DATA_TRACE_HPP
