/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CFG_HPP
#define _JACQUES_CFG_HPP

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>

namespace jacques {

class CliError final :
    public std::runtime_error
{
public:
    explicit CliError(const std::string& msg);
};

/*
 * This object guarantees:
 *
 * * There's at least one file path.
 * * There are no duplicate file paths.
 * * All file paths exist.
 */
class Cfg
{
protected:
    explicit Cfg() noexcept = default;

public:
    virtual ~Cfg() = default;
};

class InspectCfg final :
    public Cfg
{
public:
    explicit InspectCfg(std::vector<boost::filesystem::path> paths);

    const std::vector<boost::filesystem::path>& paths() const noexcept
    {
        return _paths;
    }

private:
    const std::vector<boost::filesystem::path> _paths;
};

class SinglePathCfg :
    public Cfg
{
protected:
    explicit SinglePathCfg(boost::filesystem::path&& path);

public:
    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

private:
    const boost::filesystem::path _path;
};

class PrintMetadataTextCfg final :
    public SinglePathCfg
{
public:
    explicit PrintMetadataTextCfg(boost::filesystem::path path);
};

class ListPktsCfg final :
    public SinglePathCfg
{
public:
    enum class Fmt {
        MACHINE,
    };

public:
    explicit ListPktsCfg(boost::filesystem::path path, Fmt format, bool withHeader);

    Fmt format() const noexcept
    {
        return _fmt;
    }

    bool withHeader() const noexcept
    {
        return _withHeader;
    }

private:
    Fmt _fmt;
    bool _withHeader;
};

class CopyPktsCfg final :
    public Cfg
{
public:
    explicit CopyPktsCfg(boost::filesystem::path srcPath, std::string pktIndexes,
                         boost::filesystem::path dstPath);

    const boost::filesystem::path& srcPath() const noexcept
    {
        return _srcPath;
    }

    const boost::filesystem::path& dstPath() const noexcept
    {
        return _dstPath;
    }

    const std::string& pktIndexes() const noexcept
    {
        return _pktIndexes;
    }

private:
    boost::filesystem::path _srcPath;
    std::string _pktIndexes;
    boost::filesystem::path _dstPath;
};

class CreateLttngIndexCfg final :
    public Cfg
{
public:
    explicit CreateLttngIndexCfg(std::vector<boost::filesystem::path> paths);

    const std::vector<boost::filesystem::path>& paths() const noexcept
    {
        return _paths;
    }

private:
    const std::vector<boost::filesystem::path> _paths;
};

class PrintCliUsageCfg final :
    public Cfg
{
public:
    explicit PrintCliUsageCfg() noexcept = default;
};

class PrintVersionCfg final :
    public Cfg
{
public:
    explicit PrintVersionCfg() noexcept = default;
};

std::unique_ptr<const Cfg> cfgFromArgs(int argc, const char *argv[]);

} // namespace jacques

#endif // _JACQUES_CFG_HPP
