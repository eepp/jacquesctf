/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <optional>

#include "view.hpp"

namespace jacques {

class InputView :
    public View
{
protected:
    explicit InputView(const Rect& rect, const Stylist& stylist);

public:
    virtual ~InputView() = default;

    /*
     * Sets the current text to `text`.
     */
    void text(std::string text)
    {
        _text = std::move(text);
        this->_invalidate();
    }

protected:
    virtual void _drawCurText(const std::string& text) = 0;
    void _drawBorder() const;

private:
    void _redrawContent() override;

private:
    std::string _text;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_INPUT_VIEW_HPP
