/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_INPUT_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_INPUT_VIEW_HPP

#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "view.hpp"

namespace jacques {

class InputView :
    public View
{
protected:
    explicit InputView(const Rectangle& rect, const Stylist& stylist);

public:
    virtual ~InputView();
    void drawCurrentText(const std::string& text);

protected:
    virtual void _drawCurrentText(const std::string& text) = 0;
    void _drawBorder() const;

private:
    void _redrawContent() override;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_INPUT_VIEW_HPP
