/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_IO_ERROR_HPP
#define _JACQUES_IO_ERROR_HPP

#include <stdexcept>
#include <string>

namespace jacques {

class IOError final :
    public std::runtime_error
{
public:
    explicit IOError(const boost::filesystem::path& path,
                     const std::string& msg) :
        std::runtime_error {msg},
        _path {path}
    {
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

private:
    const boost::filesystem::path _path;
};

} // namespace jacques

#endif // _JACQUES_IO_ERROR_HPP
