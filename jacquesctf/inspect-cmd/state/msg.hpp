/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_MSG_HPP
#define _JACQUES_INSPECT_CMD_STATE_MSG_HPP

namespace jacques {

enum class Message {
    ACTIVE_DS_FILE_CHANGED,
    ACTIVE_PKT_CHANGED,
    CUR_OFFSET_IN_PKT_CHANGED,
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_MSG_HPP
