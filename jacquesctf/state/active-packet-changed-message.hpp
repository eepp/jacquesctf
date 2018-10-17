/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_ACTIVE_PACKET_CHANGED_MESSAGE_HPP
#define _JACQUES_ACTIVE_PACKET_CHANGED_MESSAGE_HPP

#include "aliases.hpp"
#include "message.hpp"

namespace jacques {

class MessageVisitor;
class DataStreamFileState;

class ActivePacketChangedMessage :
    public Message
{
protected:
    void _acceptVisitor(MessageVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_ACTIVE_PACKET_CHANGED_MESSAGE_HPP
