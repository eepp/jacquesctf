/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "../../state/state.hpp"

namespace jacques {

class EventRecordTableView :
    public TableView
{
public:
    explicit EventRecordTableView(const Rectangle& rect,
                                  const Stylist& stylist, State& state);
    void timestampFormatMode(TimestampFormatMode tsFormatMode);
    void dataSizeFormatMode(utils::SizeFormatMode dsFormatMode);

protected:
    void _drawRow(Index index) override;
    bool _hasIndex(Index index) override;
    void _resized() override;
    void _selectLast() override;
    void _stateChanged(Message msg) override;

private:
    void _setColumnDescriptions();
    void _resetRow(const std::vector<TableViewColumnDescription>& descrs);

private:
    State * const _state;
    std::vector<std::unique_ptr<TableViewCell>> _row;
    const ViewStateObserverGuard _stateObserverGuard;
    TimestampFormatMode _tsFormatMode = TimestampFormatMode::LONG;
    utils::SizeFormatMode _sizeFormatMode = utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TABLE_VIEW_HPP
