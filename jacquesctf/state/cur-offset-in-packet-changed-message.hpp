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
public:
    explicit CurOffsetInPacketChangedMessage(Index oldOffsetBits,
                                             Index newOffsetBits);

    Index oldOffsetBits() const
    {
        return _oldOffsetBits;
    }

    Index newOffsetBits() const
    {
        return _newOffsetBits;
    }

protected:
    void _acceptVisitor(MessageVisitor& visitor) override;

private:
    const Index _oldOffsetBits;
    const Index _newOffsetBits;
};

} // namespace jacques

#endif // _JACQUES_CUR_OFFSET_IN_PACKET_CHANGED_MESSAGE_HPP
