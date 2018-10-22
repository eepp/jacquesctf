/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_SEARCH_INPUT_VIEW_HPP
#define _JACQUES_SEARCH_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "input-view.hpp"

namespace jacques {

class SearchInputView :
    public InputView
{
public:
    explicit SearchInputView(const Rectangle& rect,
                             const Stylist& stylist);

private:
    void _drawFormatText(const std::string& text);
    void _drawCurrentText(const std::string& text) override;
    void _drawNumber(std::string::const_iterator it,
                     std::string::const_iterator endIt);
};

} // namespace jacques

#endif // _JACQUES_SEARCH_INPUT_VIEW_HPP
