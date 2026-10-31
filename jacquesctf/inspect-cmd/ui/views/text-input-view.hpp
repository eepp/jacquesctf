/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_TEXT_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_TEXT_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <optional>

#include "input-view.hpp"

namespace jacques {

class TextInputView final :
    public InputView
{
public:
    explicit TextInputView(const Rect& rect, const Stylist& stylist);

private:
    void _drawCurText(const std::string& text) override;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_TEXT_INPUT_VIEW_HPP
