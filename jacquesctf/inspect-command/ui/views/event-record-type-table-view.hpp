/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TYPE_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TYPE_TABLE_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include <yactfr/metadata/fwd.hpp>

#include "table-view.hpp"
#include "../../state/state.hpp"

namespace jacques {

class EventRecordTypeTableView :
    public TableView
{
public:
    explicit EventRecordTypeTableView(const Rectangle& rect,
                                      const Stylist& stylist,
                                      const State& state);
    void dataStreamType(const yactfr::DataStreamType& dst);
    const yactfr::EventRecordType *eventRecordType() const;
    void selectEventRecordType(const std::string& pattern,
                               bool relative = false);
    void selectEventRecordType(yactfr::TypeId id);

    Size eventRecordTypeCount() const
    {
        if (!_eventRecordTypes) {
            return 0;
        }

        return _eventRecordTypes->size();
    }

    bool isEmpty() const
    {
        return this->eventRecordTypeCount() == 0;
    }

protected:
    void _drawRow(Index index) override;
    bool _hasIndex(Index index) override;
    void _selectLast() override;
    void _resized() override;

private:
    void _setColumnDescriptions();
    void _buildRows(const State& state);
    void _resetRow(const std::vector<TableViewColumnDescription>& descrs);
    void _buildLogLevelNames();

private:
    using EventRecordTypes = std::vector<const yactfr::EventRecordType *>;

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    std::unordered_map<const yactfr::DataStreamType *, EventRecordTypes> _rows;
    const EventRecordTypes *_eventRecordTypes = nullptr;
    std::array<std::string, 15> _logLevelNames;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_EVENT_RECORD_TYPE_TABLE_VIEW_HPP
