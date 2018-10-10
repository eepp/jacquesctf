/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "message.hpp"
#include "message-visitor.hpp"
#include "active-packet-changed-message.hpp"

namespace jacques {

ActivePacketChangedMessage::ActivePacketChangedMessage(DataStreamFileState& dataStreamFileState,
                                                       const Index newActivePacketIndex) :
    _dataStreamFileState {&dataStreamFileState},
    _newActivePacketIndex {newActivePacketIndex}
{
}

void ActivePacketChangedMessage::_acceptVisitor(MessageVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
