/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_MESSAGE_HPP
#define _JACQUES_INSPECT_COMMAND_MESSAGE_HPP

namespace jacques {

enum class Message {
    ACTIVE_DATA_STREAM_FILE_CHANGED,
    ACTIVE_PACKET_CHANGED,
    CUR_OFFSET_IN_PACKET_CHANGED,
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_MESSAGE_HPP
