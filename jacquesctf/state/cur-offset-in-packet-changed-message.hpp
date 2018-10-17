/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CUR_OFFSET_IN_PACKET_CHANGED_MESSAGE_HPP
#define _JACQUES_CUR_OFFSET_IN_PACKET_CHANGED_MESSAGE_HPP

#include "aliases.hpp"
#include "message.hpp"

namespace jacques {

class MessageVisitor;

class CurOffsetInPacketChangedMessage :
    public Message
{
protected:
    void _acceptVisitor(MessageVisitor& visitor) override;
};

} // namespace jacques

#endif // _JACQUES_CUR_OFFSET_IN_PACKET_CHANGED_MESSAGE_HPP
