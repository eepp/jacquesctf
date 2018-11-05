/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_TYPE_EXPLORER_VIEW_HPP
#define _JACQUES_DATA_TYPE_EXPLORER_VIEW_HPP

#include <yactfr/metadata/fwd.hpp>
#include <boost/variant.hpp>

#include "scroll-view.hpp"
#include "metadata.hpp"
#include "abstract-data-type-details.hpp"

namespace jacques {

class DataTypeExplorerView :
    public ScrollView
{
public:
    explicit DataTypeExplorerView(const Rectangle& rect,
                                  const Stylist& stylist);
    void dataStreamType(const yactfr::DataStreamType& dataStreamType);
    void eventRecordType(const yactfr::EventRecordType& eventRecordType);
    void singleDataType(const yactfr::DataType& dataType, yactfr::Scope scope);
    void highlightDataType(const yactfr::DataType& dataType);
    void reset();
    void clearHighlight();
    void centerHighlight();

private:
    struct _ScopeSubtitleRow
    {
        yactfr::Scope scope;
    };

    struct _EmptyRow
    {
    };

private:
    using _Details = std::vector<AbstractDataTypeDetails::UP>;
    using _Row = boost::variant<_ScopeSubtitleRow, _EmptyRow,
                                const AbstractDataTypeDetails *>;

private:
    void _drawRows() override;
    void _drawScopeSubtitleRow(Index index, const std::string& text);
    void _appendDetailsRow(const yactfr::DataType& dataType,
                           _Details& details);

private:
    const yactfr::EventRecordType *_eventRecordType = nullptr;
    const yactfr::DataType *_singleDataType = nullptr;
    const yactfr::DataType *_highlight = nullptr;

    struct {
        _Details pktHeader;
        _Details pktContext;
        _Details ertHeader;
        _Details ertFirstCtx;
        _Details ertSecondCtx;
        _Details ertPayload;
    } _details;

    _Details _singleDataTypeDetails;
    std::vector<_Row> _rows;
};

} // namespace jacques

#endif // _JACQUES_DATA_TYPE_EXPLORER_VIEW_HPP
