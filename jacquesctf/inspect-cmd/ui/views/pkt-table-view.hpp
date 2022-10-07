/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "../../state/state.hpp"
#include "data/data-len.hpp"

namespace jacques {

class PktTableView final :
    public TableView
{
public:
    explicit PktTableView(const Rect& rect, const Stylist& stylist, State& state);
    void tsFmtMode(TsFmtMode tsFmtMode);
    void dataLenFmtMode(utils::LenFmtMode dataLenFmtMode);
    Index selPktIndex() const noexcept;
    void selPktIndex(Index index);

protected:
    void _drawRow(Index row) override;
    Size _rowCount() override;
    void _resized() override;
    void _stateChanged(Message msg) override;

private:
    void _setColumnDescrs();
    void _resetRow(const std::vector<TableViewColumnDescr>& descrs);

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
    TsFmtMode _tsFmtMode = TsFmtMode::LONG;
    utils::LenFmtMode _dataLenFmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_TABLE_VIEW_HPP
