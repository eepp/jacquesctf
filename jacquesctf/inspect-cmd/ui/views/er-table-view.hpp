/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "../../state/inspect-cmd-state.hpp"

namespace jacques {

class ErTableView final :
    public TableView
{
public:
    explicit ErTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);
    void tsFmtMode(TsFmtMode tsFmtMode);
    void dataLenFmtMode(utils::LenFmtMode dataLenFmtMode);

protected:
    void _drawRow(Index index) override;
    Size _rowCount() override;
    void _resized() override;
    void _appStateChanged(Message msg) override;

private:
    void _setColumnDescrs();
    void _resetRow(const std::vector<TableViewColumnDescr>& descrs);

private:
    InspectCmdState *_appState;
    ViewInspectCmdStateObserverGuard _appStateObserverGuard;
    std::vector<std::unique_ptr<TableViewCell>> _row;
    TsFmtMode _tsFmtMode = TsFmtMode::LONG;
    utils::LenFmtMode _dataLenFmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
