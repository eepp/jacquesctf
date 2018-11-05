/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CONFIG_HPP
#define _JACQUES_CONFIG_HPP

#include <string>
#include <list>
#include <stdexcept>
#include <boost/filesystem.hpp>

namespace jacques {

class CliError :
    public std::runtime_error
{
public:
    CliError(const std::string& msg);
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
public:
	enum class Command
	{
		INSPECT_FILES,
		PRINT_METADATA_TEXT,
        PRINT_CLI_USAGE,
        PRINT_VERSION,
	};

public:
	explicit Config(int argc, const char *argv[]);

    Command command() const noexcept
    {
        return _cmd;
    }

    const std::list<boost::filesystem::path>& filePaths() const noexcept
    {
        return _filePaths;
    }

    bool enableLogging() const noexcept
    {
        return _enableLogging;
    }

private:
    void _parseArgs(int argc, const char *argv[]);
    void _expandPaths();

private:
	Command _cmd = Command::INSPECT_FILES;
	std::list<boost::filesystem::path> _filePaths;
    bool _enableLogging = false;
};

} // namespace jacques

#endif // _JACQUES_CONFIG_HPP
