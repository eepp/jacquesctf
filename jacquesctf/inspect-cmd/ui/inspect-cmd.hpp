/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_INSPECT_COMMAND_HPP
#define _JACQUES_INSPECT_COMMAND_UI_INSPECT_COMMAND_HPP

#include <stdexcept>

namespace jacques {

enum class KeyHandlingReaction
{
    CONTINUE,
    RETURN_TO_INSPECT,
    RETURN_TO_PKTS,
};

class InspectCfg;

/*
 * Stats the interactive (ncurses) part of Jacques CTF.
 *
 * This takes control of the terminal and returns only when the user
 * manually quits. The function eventually registers a handler for the
 * SIGINT signal so that the program quits immediately.
 */
void inspectCmd(const InspectCfg& cfg);

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_INSPECT_COMMAND_HPP
