/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_MESSAGE_HPP
#define _JACQUES_MESSAGE_HPP

namespace jacques {

class MessageVisitor;

class Message
{
protected:
    virtual void _acceptVisitor(MessageVisitor& visitor) = 0;
};

} // namespace jacques

#endif // _JACQUES_MESSAGE_HPP
