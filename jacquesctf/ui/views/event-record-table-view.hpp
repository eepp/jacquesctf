/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_EVENT_RECORD_TABLE_VIEW_HPP
#define _JACQUES_EVENT_RECORD_TABLE_VIEW_HPP

#include "table-view.hpp"
#include "state.hpp"

namespace jacques {

class EventRecordTableView :
    public TableView
{
public:
    explicit EventRecordTableView(const Rectangle& rect,
                                  std::shared_ptr<const Stylist> stylist,
                                  std::shared_ptr<State> state);
    void timestampFormatMode(TimestampFormatMode tsFormatMode);
    void dataSizeFormatMode(utils::SizeFormatMode dsFormatMode);

protected:
    void _drawRow(Index index) override;
    bool _hasIndex(Index index) override;
    void _resized() override;
    void _selectLast() override;
    void _stateChanged(const Message& msg) override;

private:
    void _setColumnDescriptions();
    void _resetRow(const std::vector<TableViewColumnDescription>& descrs);

private:
    std::shared_ptr<State> _state;
    std::vector<std::unique_ptr<TableViewCell>> _row;
    const ViewStateObserverGuard _stateObserverGuard;
    TimestampFormatMode _tsFormatMode = TimestampFormatMode::LONG;
    utils::SizeFormatMode _sizeFormatMode = utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS;
};

} // namespace jacques

#endif // _JACQUES_EVENT_RECORD_TABLE_VIEW_HPP
