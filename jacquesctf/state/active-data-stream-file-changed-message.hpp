/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_ACTIVE_DATA_STREAM_FILE_CHANGED_MESSAGE_HPP
#define _JACQUES_ACTIVE_DATA_STREAM_FILE_CHANGED_MESSAGE_HPP

#include "aliases.hpp"
#include "message.hpp"

namespace jacques {

class MessageVisitor;
class DataStreamFileState;

class ActiveDataStreamFileChangedMessage :
    public Message
{
public:
    explicit ActiveDataStreamFileChangedMessage(Index newDataStreamFileIndex);

    Index newDataStreamFileStateIndex() const
    {
        return _newDataStreamFileIndex;
    }

protected:
    void _acceptVisitor(MessageVisitor& visitor) override;

private:
    const Index _newDataStreamFileIndex;
};

} // namespace jacques

#endif // _JACQUES_ACTIVE_DATA_STREAM_FILE_CHANGED_MESSAGE_HPP
