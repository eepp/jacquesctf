/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <optional>

#include "input-view.hpp"

namespace jacques {

class SearchInputView final :
    public InputView
{
public:
    explicit SearchInputView(const Rect& rect, const Stylist& stylist);
    void animateBorder(Index index);

private:
    void _drawFormatText(const std::string& text);
    void _drawCurText(const std::string& text) override;
    void _drawNumber(std::u32string::const_iterator it, std::u32string::const_iterator endIt);
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
