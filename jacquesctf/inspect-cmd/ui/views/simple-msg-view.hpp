/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
    void msg(std::string_view msg);

protected:
    void _redrawContent() override;

private:
    std::string _msg;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SIMPLE_MSG_VIEW_HPP
