/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "../../state/state.hpp"

namespace jacques {

class ErTableView final :
    public TableView
{
public:
    explicit ErTableView(const Rect& rect, const Stylist& stylist, State& state);
    void tsFmtMode(TsFmtMode tsFmtMode);
    void dataLenFmtMode(utils::LenFmtMode dataLenFmtMode);

protected:
    void _drawRow(Index index) override;
    Size _rowCount() override;
    void _resized() override;
    void _stateChanged(Message msg) override;

private:
    void _setColumnDescrs();
    void _resetRow(const std::vector<TableViewColumnDescr>& descrs);

private:
    State * const _state;
    std::vector<std::unique_ptr<TableViewCell>> _row;
    const ViewStateObserverGuard _stateObserverGuard;
    TsFmtMode _tsFmtMode = TsFmtMode::LONG;
    utils::LenFmtMode _dataLenFmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ER_TABLE_VIEW_HPP
