/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <boost/optional.hpp>

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
    void _drawNumber(std::string::const_iterator it, std::string::const_iterator endIt);
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
