/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "simple-msg-view.hpp"

namespace jacques {

SimpleMsgView::SimpleMsgView(const Rect& rect, const Stylist& stylist) :
    View {rect, "A message for you", DecorationStyle::BORDERS_EMPHASIZED, stylist}
{
}

void SimpleMsgView::msg(const std::string_view msg)
{
    _msg = utils::escapeStr(msg);
    this->_invalidate();
}

void SimpleMsgView::_redrawContent()
{
    this->_clearContent();
    this->_write({1, 1}, _msg);
}

} // namespace jacques
