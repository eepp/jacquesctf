/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../views/ds-file-table-view.hpp"
#include "screen.hpp"

namespace jacques {

class DsFilesScreen final :
    public Screen
{
public:
    explicit DsFilesScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                           ScreenHost& host);

private:
    void _handleKey(Key key) override;

private:
    std::unique_ptr<DsFileTableView> _view;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP
