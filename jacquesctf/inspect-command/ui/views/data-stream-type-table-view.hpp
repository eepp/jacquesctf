/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_STREAM_TYPE_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_STREAM_TYPE_TABLE_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <yactfr/metadata/fwd.hpp>

#include "table-view.hpp"
#include "../../state/state.hpp"

namespace jacques {

class DataStreamTypeTableView :
    public TableView
{
public:
    explicit DataStreamTypeTableView(const Rectangle& rect,
                                     const Stylist& stylist,
                                     const State& state);
    void traceType(const yactfr::TraceType& traceType);
    const yactfr::DataStreamType *dataStreamType() const;
    void selectDataStreamType(yactfr::TypeId id);

protected:
    void _drawRow(Index index) override;
    bool _hasIndex(Index index) override;
    void _selectLast() override;
    void _resized() override;

private:
    void _setColumnDescriptions();
    void _buildRows(const State& state);

private:
    using DataStreamTypes = std::vector<const yactfr::DataStreamType *>;

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    std::unordered_map<const yactfr::TraceType *, DataStreamTypes> _rows;
    const DataStreamTypes *_dataStreamTypes = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_STREAM_TYPE_TABLE_VIEW_HPP
