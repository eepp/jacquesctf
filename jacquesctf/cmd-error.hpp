/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
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
