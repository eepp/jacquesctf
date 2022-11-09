/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../views/ds-file-table-view.hpp"
#include "screen.hpp"
#include "../cycle-wheel.hpp"
#include "data/data-len.hpp"

namespace jacques {

class DsFilesScreen final :
    public Screen
{
public:
    explicit DsFilesScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                           InspectCmdState& appState);

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<DsFileTableView> _view;
    CycleWheel<TsFmtMode> _tsFmtModeWheel;
    CycleWheel<utils::LenFmtMode> _dataLenFmtModeWheel;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_DS_FILES_SCREEN_HPP
