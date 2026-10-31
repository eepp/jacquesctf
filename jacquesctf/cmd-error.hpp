/*
 * SPDX-FileCopyrightText: 2019 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_CMD_ERROR_HPP
#define _JACQUES_CMD_ERROR_HPP

#include <stdexcept>
#include <string>

namespace jacques {

class CmdError final :
    public std::runtime_error
{
public:
    explicit CmdError(const std::string& msg) :
        std::runtime_error {msg}
    {
    }
};

} // namespace jacques

#endif // _JACQUES_CMD_ERROR_HPP
