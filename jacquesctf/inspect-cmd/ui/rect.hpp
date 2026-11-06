/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_RECT_HPP
#define _JACQUES_INSPECT_COMMAND_UI_RECT_HPP

#include "aliases.hpp"
#include "point.hpp"

namespace jacques {

struct Rect final
{
    Point pos;
    Size w, h;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_RECT_HPP
