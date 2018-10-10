/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "message-visitor.hpp"

namespace jacques {

void MessageVisitor::visit(ActiveDataStreamFileChangedMessage& msg)
{
}

void MessageVisitor::visit(ActivePacketChangedMessage& msg)
{
}

void MessageVisitor::visit(CurOffsetInPacketChangedMessage& msg)
{
}

} // namespace jacques
