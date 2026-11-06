/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <optional>

#include "view.hpp"

namespace jacques {

class SearchInputView final :
    public View
{
public:
    explicit SearchInputView(const Rect& rect, const Stylist& stylist);

    /*
     * Sets the current text to `text`.
     */
    void text(std::string text)
    {
        _text = std::move(text);
        this->_invalidate();
    }

    /*
     * Draws the frame `index` of the border animation.
     *
     * Unlike the other view methods, this one draws immediately: call
     * term::update() afterwards (not render(); see
     * SearchCtrl::animateWhile()).
     */
    void animateBorder(Index index);

private:
    void _redrawContent() override;
    void _drawBorder() const;
    void _drawFormatText(Point contentPos, const std::string& text);
    void _drawNumber(Point& contentPos, std::u32string::const_iterator it,
                     std::u32string::const_iterator endIt);
    void _writeCh(Point& contentPos, char32_t ch) const;

private:
    std::string _text;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SEARCH_INPUT_VIEW_HPP
