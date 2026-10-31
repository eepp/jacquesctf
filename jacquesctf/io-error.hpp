/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
    explicit IOError(boost::filesystem::path path, const std::string& msg) :
        std::runtime_error {msg},
        _path {std::move(path)}
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
