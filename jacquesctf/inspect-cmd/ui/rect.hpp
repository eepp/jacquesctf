/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_RECT_HPP
#define _JACQUES_INSPECT_COMMAND_UI_RECT_HPP

#include "aliases.hpp"
#include "point.hpp"

namespace jacques {

struct Rect
{
    Point pos;
    Size w, h;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_RECT_HPP
