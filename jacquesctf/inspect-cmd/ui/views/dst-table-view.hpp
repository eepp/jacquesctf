/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DST_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DST_TABLE_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <yactfr/metadata/fwd.hpp>

#include "table-view.hpp"
#include "../../state/state.hpp"

namespace jacques {

class DstTableView final :
    public TableView
{
public:
    explicit DstTableView(const Rect& rect, const Stylist& stylist, const State& state);
    void traceType(const yactfr::TraceType& traceType);
    const yactfr::DataStreamType *dst() const;
    void selectDst(yactfr::TypeId id);

protected:
    void _drawRow(Index index) override;
    Size _rowCount() override;
    void _resized() override;

private:
    void _setColumnDescrs();
    void _buildRows(const State& state);

private:
    using Dsts = std::vector<const yactfr::DataStreamType *>;

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    std::unordered_map<const yactfr::TraceType *, Dsts> _rows;
    const Dsts *_dsts = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DST_TABLE_VIEW_HPP
