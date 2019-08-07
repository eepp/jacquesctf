/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CONFIG_HPP
#define _JACQUES_CONFIG_HPP

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>

namespace jacques {

class CliError :
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
class Config
{
protected:
    Config();

public:
    virtual ~Config();
};

class InspectConfig :
    public Config
{
public:
    explicit InspectConfig(std::vector<boost::filesystem::path>&& paths);

    const std::vector<boost::filesystem::path>& paths() const noexcept
    {
        return _paths;
    }

private:
    const std::vector<boost::filesystem::path> _paths;
};

class SinglePathConfig :
    public Config
{
protected:
    explicit SinglePathConfig(const boost::filesystem::path& path);

public:
    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

private:
    const boost::filesystem::path _path;
};

class PrintMetadataTextConfig :
    public SinglePathConfig
{
public:
    explicit PrintMetadataTextConfig(const boost::filesystem::path& path);
};

class ListPacketsConfig :
    public SinglePathConfig
{
public:
    enum class Format {
        MACHINE,
    };

public:
    explicit ListPacketsConfig(const boost::filesystem::path& path,
                               Format format, bool withHeader);

    Format format() const noexcept
    {
        return _format;
    }

    bool withHeader() const noexcept
    {
        return _withHeader;
    }

private:
    Format _format;
    bool _withHeader;
};

class CopyPacketsConfig :
    public Config
{
public:
    explicit CopyPacketsConfig(const boost::filesystem::path& srcPath,
                               const std::string& packetIndexes,
                               const boost::filesystem::path& dstPath);

    const boost::filesystem::path& srcPath() const noexcept
    {
        return _srcPath;
    }

    const boost::filesystem::path& dstPath() const noexcept
    {
        return _dstPath;
    }

    const std::string& packetIndexes() const noexcept
    {
        return _packetIndexes;
    }

private:
    boost::filesystem::path _srcPath;
    std::string _packetIndexes;
    boost::filesystem::path _dstPath;
};

class CreateLttngIndexConfig :
    public Config
{
public:
    explicit CreateLttngIndexConfig(std::vector<boost::filesystem::path>&& paths);

    const std::vector<boost::filesystem::path>& paths() const noexcept
    {
        return _paths;
    }

private:
    const std::vector<boost::filesystem::path> _paths;
};

class PrintCliUsageConfig :
    public Config
{
public:
    PrintCliUsageConfig();
};

class PrintVersionConfig :
    public Config
{
public:
    PrintVersionConfig();
};

std::unique_ptr<const Config> configFromArgs(int argc, const char *argv[]);

} // namespace jacques

#endif // _JACQUES_CONFIG_HPP
