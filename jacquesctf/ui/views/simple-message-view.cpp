/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "simple-message-view.hpp"

namespace jacques {

SimpleMessageView::SimpleMessageView(const Rectangle& rect,
                                     std::shared_ptr<const Stylist> stylist) :
    View {
        rect, "A message for you",
        DecorationStyle::BORDERS_EMPHASIZED, stylist
    }
{
}

void SimpleMessageView::message(const std::string& msg)
{
    _msg = msg;
    this->_redrawContent();
}

void SimpleMessageView::_redrawContent()
{
    this->_clearContent();
    this->_safeMoveAndPrint({1, 1}, "%s", _msg.c_str());
}

void SimpleMessageView::_resized()
{
    // TODO
}

} // namespace jacques
