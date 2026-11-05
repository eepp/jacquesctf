/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_MSG_HPP
#define _JACQUES_INSPECT_CMD_STATE_MSG_HPP

namespace jacques {

enum class Message {
    ACTIVE_DS_FILE_AND_PKT_CHANGED,
    ACTIVE_PKT_CHANGED,
    CUR_OFFSET_IN_PKT_CHANGED,

    // timestamp or data length format mode changed
    FMT_MODES_CHANGED,
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_MSG_HPP
