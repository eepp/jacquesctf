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
public:
    explicit ActivePacketChangedMessage(DataStreamFileState& dataStreamFileState,
                                        Index newActivePacketIndex);

    DataStreamFileState& dataStreamFileState() const
    {
        return *_dataStreamFileState;
    }

    Index newActivePacketIndex() const
    {
        return _newActivePacketIndex;
    }

protected:
    void _acceptVisitor(MessageVisitor& visitor) override;

private:
    DataStreamFileState * const _dataStreamFileState;
    const Index _newActivePacketIndex;
};

} // namespace jacques

#endif // _JACQUES_ACTIVE_PACKET_CHANGED_MESSAGE_HPP
