/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_SIMPLE_MESSAGE_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_SIMPLE_MESSAGE_VIEW_HPP

#include "view.hpp"

namespace jacques {

class SimpleMessageView :
    public View
{
public:
    explicit SimpleMessageView(const Rectangle& rect, const Stylist& stylist);
    void message(const std::string& msg);

protected:
    void _resized() override;
    void _redrawContent() override;

private:
    std::string _msg;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_SIMPLE_MESSAGE_VIEW_HPP
