/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_TEXT_INPUT_VIEW_HPP
#define _JACQUES_TEXT_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "input-view.hpp"

namespace jacques {

class TextInputView :
    public InputView
{
public:
    explicit TextInputView(const Rectangle& rect,
                           const Stylist& stylist);

private:
    void _drawCurrentText(const std::string& text) override;
};

} // namespace jacques

#endif // _JACQUES_TEXT_INPUT_VIEW_HPP
