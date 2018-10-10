/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "message.hpp"
#include "message-visitor.hpp"
#include "active-data-stream-file-changed-message.hpp"

namespace jacques {

ActiveDataStreamFileChangedMessage::ActiveDataStreamFileChangedMessage(const Index newDataStreamFileIndex) :
    _newDataStreamFileIndex {newDataStreamFileIndex}
{
}

void ActiveDataStreamFileChangedMessage::_acceptVisitor(MessageVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
