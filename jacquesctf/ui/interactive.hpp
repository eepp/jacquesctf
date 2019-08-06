/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INTERACTIVE_HPP
#define _JACQUES_INTERACTIVE_HPP

namespace jacques {

enum class KeyHandlingReaction
{
    CONTINUE,
    RETURN_TO_INSPECT,
    RETURN_TO_PACKETS,
};

class InspectConfig;

/*
 * Stats the interactive (ncurses) part of Jacques CTF.
 *
 * This takes control of the terminal and returns only when the user
 * manually quits. The function eventually registers a handler for the
 * SIGINT signal so that the program quits immediately.
 */
bool startInteractive(const InspectConfig& cfg);

} // namespace jacques

#endif // _JACQUES_INTERACTIVE_HPP
