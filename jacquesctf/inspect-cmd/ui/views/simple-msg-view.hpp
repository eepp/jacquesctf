/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SIMPLE_MSG_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SIMPLE_MSG_VIEW_HPP

#include "view.hpp"

namespace jacques {

class SimpleMsgView final :
    public View
{
public:
    explicit SimpleMsgView(const Rect& rect, const Stylist& stylist);
    void msg(const std::string& msg);

protected:
    void _resized() override;
    void _redrawContent() override;

private:
    std::string _msg;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SIMPLE_MSG_VIEW_HPP
