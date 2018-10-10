/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_MESSAGE_VISITOR_HPP
#define _JACQUES_MESSAGE_VISITOR_HPP

#include "aliases.hpp"

namespace jacques {

class ActiveDataStreamFileChangedMessage;
class ActivePacketChangedMessage;
class CurOffsetInPacketChangedMessage;

class MessageVisitor
{
public:
    virtual void visit(ActiveDataStreamFileChangedMessage& msg);
    virtual void visit(ActivePacketChangedMessage& msg);
    virtual void visit(CurOffsetInPacketChangedMessage& msg);
};

} // namespace jacques

#endif // _JACQUES_MESSAGE_VISITOR_HPP
