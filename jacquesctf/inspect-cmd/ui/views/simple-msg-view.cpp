/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "simple-msg-view.hpp"

namespace jacques {

SimpleMsgView::SimpleMsgView(const Rect& rect, const Stylist& stylist) :
    View {rect, "A message for you", DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void SimpleMsgView::msg(const std::string& msg)
{
    _msg = utils::escapeStr(msg);
    this->_redrawContent();
}

void SimpleMsgView::_redrawContent()
{
    this->_clearContent();
    this->_safeMoveAndPrint({1, 1}, "%s", _msg.c_str());
}

void SimpleMsgView::_resized()
{
    // TODO
}

} // namespace jacques
