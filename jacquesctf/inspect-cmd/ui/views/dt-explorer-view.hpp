/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_EXPLORER_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_EXPLORER_VIEW_HPP

#include <yactfr/metadata/fwd.hpp>
#include <boost/variant.hpp>

#include "scroll-view.hpp"
#include "data/metadata.hpp"
#include "abstract-dt-details.hpp"

namespace jacques {

class DtExplorerView :
    public ScrollView
{
public:
    explicit DtExplorerView(const Rect& rect, const Stylist& stylist);
    void dst(const yactfr::DataStreamType& dst, bool showErts = true);
    void ert(const yactfr::EventRecordType& ert);
    void singleDt(const yactfr::DataType& dt, yactfr::Scope scope);
    void highlightDt(const yactfr::DataType& dt);
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
    using _Details = std::vector<AbstractDtDetails::UP>;
    using _Row = boost::variant<_ScopeSubtitleRow, _EmptyRow, const AbstractDtDetails *>;

private:
    void _drawRows() override;
    void _drawScopeSubtitleRow(Index index, const std::string& text);
    void _appendDetailsRow(const yactfr::DataType& dt, _Details& details);

private:
    const yactfr::EventRecordType *_ert = nullptr;
    const yactfr::DataType *_singleDt = nullptr;
    const yactfr::DataType *_highlight = nullptr;

    struct {
        _Details pktHeader;
        _Details pktContext;
        _Details ertHeader;
        _Details ertFirstCtx;
        _Details ertSecondCtx;
        _Details ertPayload;
    } _details;

    _Details _singleDtDetails;
    std::vector<_Row> _rows;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DT_EXPLORER_VIEW_HPP
