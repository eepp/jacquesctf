/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_ERT_TABLE_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_ERT_TABLE_VIEW_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include <yactfr/metadata/fwd.hpp>

#include "table-view.hpp"

namespace jacques {

class ErtTableView final :
    public TableView
{
public:
    explicit ErtTableView(const Rect& rect, const Stylist& stylist, const InspectCmdState& appState);
    void dst(const yactfr::DataStreamType& dst);
    const yactfr::EventRecordType *ert() const noexcept;
    void selectErt(std::string_view pattern, bool relative = false);
    void selectErt(yactfr::TypeId id);

    Size ertCount() const noexcept
    {
        return _erts ? _erts->size() : 0;
    }

    bool isEmpty() const noexcept
    {
        return this->ertCount() == 0;
    }

protected:
    Size _rowCount() override;

private:
    void _buildRows(const InspectCmdState& appState);
    //void _buildLogLevelNames();

private:
    using Erts = std::vector<const yactfr::EventRecordType *>;

private:
    std::unordered_map<const yactfr::DataStreamType *, Erts> _rows;
    const Erts *_erts = nullptr;
    //std::array<std::string, 15> _logLevelNames;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_ERT_TABLE_VIEW_HPP
