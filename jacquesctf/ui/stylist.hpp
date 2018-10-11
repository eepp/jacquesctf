/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_STYLIST_HPP
#define _JACQUES_STYLIST_HPP

#include <curses.h>

#include "view.hpp"

namespace jacques {

class Stylist
{
public:
    Stylist();
    void viewBorder(const View& view, bool focused, bool emphasized) const;
    void viewTitle(const View& view, bool focused, bool emphasized) const;
    void viewHasMore(const View& view) const;
    void tableViewTsCellNsPart(const View& view, bool emphasized) const;
    void tableViewTextCell(const View& view, bool emphasized) const;
    void tableViewNaCell(const View& view, bool emphasized) const;
    void tableViewBoolCell(const View& view, bool value,
                           bool emphasized) const;
    void tableViewHeader(const View& view) const;
    void tableViewSelection(const View& view, bool error = false) const;
    void tableViewSelectionSep(const View& view, bool error = false) const;
    void tableViewSep(const View& view) const;
    void tableViewWarningRow(const View& view) const;
    void tableViewWarningCell(const View& view) const;
    void tableViewErrorCell(const View& view) const;
    void textMore(const View& view) const;
    void stdDim(const View& view) const;
    void std(WINDOW *window, bool emphasized = false) const;

    void std(const View& view, const bool emphasized = false) const
    {
        this->std(view._window(), emphasized);
    }

    void stdHighlight(WINDOW *window) const;

    void stdHighlight(const View& view) const
    {
        this->stdHighlight(view._window());
    }

    void helpViewSection(const View& view) const;
    void helpViewKey(const View& view) const;
    void statusViewStd(const View& view, bool emphasized = false) const;
    void statusViewFilename(const View& view) const;
    void simpleInputViewBorder(const View& view) const;
    void packetIndexBuildProgressViewPath(const View& view, bool filename) const;
    void packetIndexBuildProgressViewBar(const View& view, bool on) const;
    void detailsViewSubtitle(const View& view) const;
    void detailsViewTypeInfo(WINDOW *window) const;
    void detailsViewDataTypeName(WINDOW *window) const;
    void detailsViewEnumDataTypeMemberName(WINDOW *window) const;
    void detailsViewEnumDataTypeMemberRange(WINDOW *window) const;
    void detailsViewPropKey(WINDOW *window) const;
    void detailsViewPropValue(WINDOW *window) const;
    void traceInfoViewSection(const View& view) const;
    void traceInfoViewPropKey(const View& view) const;
    void traceInfoViewPropValue(const View& view) const;
    void packetDecodingErrorDetailsView(const View& view,
                                        bool emphasized = false) const;
    void error() const;
    void error(WINDOW *window) const;

    void error(const View& view) const
    {
        this->error(view._window());
    }

    void searchInputViewPrefix(const View& view) const;
    void searchInputViewAddSub(const View& view) const;
    void searchInputViewWildcard(const View& view) const;
    void searchInputViewEscape(const View& view) const;
    void searchInputViewNumber(const View& view) const;
    void searchInputViewError(const View& view) const;

private:
    void _initColor(int id, int fg, int bg) const;
    void _color(WINDOW *window, int pair) const;

    void _color(const View& view, const int pair) const
    {
        this->_color(view._window(), pair);
    }

    void _attrs(WINDOW *window, int attrs) const;

    void _attrs(const View& view, const int attrs) const
    {
        this->_attrs(view._window(), attrs);
    }

    void _attrsReset(WINDOW *window) const;

    void _attrsReset(const View& view) const
    {
        this->_attrsReset(view._window());
    }

private:
    enum {
        _COLOR_ID_VIEW_BORDER_FOCUSED = 1,
        _COLOR_ID_VIEW_BORDER_EMPHASIZED,
        _COLOR_ID_VIEW_BORDER_BLURRED,
        _COLOR_ID_VIEW_TITLE_FOCUSED,
        _COLOR_ID_VIEW_TITLE_EMPHASIZED,
        _COLOR_ID_VIEW_TITLE_BLURRED,
        _COLOR_ID_VIEW_HAS_MORE,
        _COLOR_ID_STD,
        _COLOR_ID_TABLE_VIEW_HEADER,
        _COLOR_ID_BOOL_YES,
        _COLOR_ID_BOOL_NO,
        _COLOR_ID_TABLE_VIEW_SELECTION,
        _COLOR_ID_TABLE_VIEW_SELECTION_ERROR,
        _COLOR_ID_TEXT_MORE,
        _COLOR_ID_TABLE_VIEW_SEP,
        _COLOR_ID_TABLE_VIEW_WARNING_CELL,
        _COLOR_ID_TABLE_VIEW_ERROR_CELL,
        _COLOR_ID_TABLE_VIEW_TEXT_CELL_EMPHASIZED,
        _COLOR_ID_TABLE_VIEW_TS_CELL_NS_PART,
        _COLOR_ID_HELP_VIEW_SECTION,
        _COLOR_ID_HELP_VIEW_KEY,
        _COLOR_ID_STATUS_VIEW_STD,
        _COLOR_ID_SIMPLE_INPUT_VIEW_BORDER,
        _COLOR_ID_PACKET_INDEX_BUILD_PROGRESS_VIEW_PATH,
        _COLOR_ID_DETAILS_VIEW_SUBTITLE,
        _COLOR_ID_DETAILS_VIEW_TYPE_INFO,
        _COLOR_ID_DETAILS_VIEW_DATA_TYPE_NAME,
        _COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_NAME,
        _COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_RANGE,
        _COLOR_ID_DETAILS_VIEW_PROP_KEY,
        _COLOR_ID_DETAILS_VIEW_PROP_VALUE,
        _COLOR_ID_TRACE_INFO_VIEW_PROP_VALUE,
        _COLOR_ID_PACKET_DECODING_ERROR_DETAILS_VIEW,
        _COLOR_ID_SEARCH_INPUT_VIEW_PREFIX,
        _COLOR_ID_SEARCH_INPUT_VIEW_ADD_SUB,
        _COLOR_ID_SEARCH_INPUT_VIEW_WILDCARD,
        _COLOR_ID_SEARCH_INPUT_VIEW_ESCAPE,
        _COLOR_ID_SEARCH_INPUT_VIEW_NUMBER,
        _COLOR_ID_SEARCH_INPUT_VIEW_ERROR,
    };
};

} // namespace jacques

#endif // _JACQUES_STYLIST_HPP
