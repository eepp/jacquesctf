/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP

#include "data/data-len.hpp"
#include "data/ts.hpp"
#include "table-view.hpp"

namespace jacques {

class DsFileTableView final :
    public TableView
{
public:
    explicit DsFileTableView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);
    Index selDsFileIndex() const;
    void selDsFileIndex(Index index);
    void tsFmtMode(TsFmtMode tsFmtMode);
    void dataLenFmtMode(utils::LenFmtMode dataLenFmtMode);

private:
    void _drawRow(Index index) override;
    Size _rowCount() override;
    void _resized() override;
    void _appStateChanged(Message msg) override;
    void _setColumnDescrs();
    void _resetRow(const std::vector<TableViewColumnDescr>& descrs);

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    InspectCmdState *_appState;
    ViewInspectCmdStateObserverGuard _appStateObserverGuard;
    TsFmtMode _tsFmtMode = TsFmtMode::LONG;
    utils::LenFmtMode _dataLenFmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DS_FILE_TABLE_VIEW_HPP
