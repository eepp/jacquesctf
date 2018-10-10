/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <curses.h>

#include "stylist.hpp"
#include "view.hpp"
#include "utils.hpp"

namespace jacques {

Stylist::Stylist()
{
    this->_initColor(_COLOR_ID_VIEW_BORDER_FOCUSED, -1, -1);
    this->_initColor(_COLOR_ID_VIEW_BORDER_BLURRED, -1, -1);
    this->_initColor(_COLOR_ID_VIEW_BORDER_EMPHASIZED, COLOR_YELLOW, -1);
    this->_initColor(_COLOR_ID_VIEW_TITLE_FOCUSED, -1, -1);
    this->_initColor(_COLOR_ID_VIEW_TITLE_BLURRED, -1, -1);
    this->_initColor(_COLOR_ID_VIEW_TITLE_EMPHASIZED, COLOR_YELLOW, -1);
    this->_initColor(_COLOR_ID_VIEW_HAS_MORE, COLOR_YELLOW, -1);
    this->_initColor(_COLOR_ID_STD, -1, -1);
    this->_initColor(_COLOR_ID_TABLE_VIEW_HEADER, COLOR_BLACK, COLOR_GREEN);
    this->_initColor(_COLOR_ID_BOOL_YES, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_BOOL_NO, COLOR_MAGENTA, -1);
    this->_initColor(_COLOR_ID_TABLE_VIEW_SELECTION, COLOR_WHITE, COLOR_CYAN);
    this->_initColor(_COLOR_ID_TABLE_VIEW_SELECTION_ERROR, COLOR_WHITE, COLOR_RED);
    this->_initColor(_COLOR_ID_TABLE_VIEW_TEXT_CELL_EMPHASIZED, COLOR_YELLOW, -1);
    this->_initColor(_COLOR_ID_TEXT_MORE, COLOR_MAGENTA, -1);
    this->_initColor(_COLOR_ID_TABLE_VIEW_SEP, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_HELP_VIEW_SECTION, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_HELP_VIEW_KEY, COLOR_CYAN, -1);
    this->_initColor(_COLOR_ID_STATUS_VIEW_STD, COLOR_WHITE, COLOR_BLUE);
    this->_initColor(_COLOR_ID_TABLE_VIEW_WARNING_CELL, COLOR_WHITE, COLOR_YELLOW);
    this->_initColor(_COLOR_ID_TABLE_VIEW_ERROR_CELL, COLOR_RED, -1);
    this->_initColor(_COLOR_ID_SIMPLE_INPUT_VIEW_BORDER, COLOR_BLACK, COLOR_GREEN);
    this->_initColor(_COLOR_ID_PACKET_INDEX_BUILD_PROGRESS_VIEW_PATH, COLOR_BLUE, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_SUBTITLE, COLOR_CYAN, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_TYPE_INFO, COLOR_MAGENTA, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_DATA_TYPE_NAME, COLOR_WHITE, COLOR_BLUE);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_NAME, COLOR_YELLOW, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_RANGE, COLOR_CYAN, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_PROP_KEY, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_DETAILS_VIEW_PROP_VALUE, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_TRACE_INFOS_VIEW_PROP_VALUE, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_PACKET_DECODING_ERROR_DETAILS_VIEW, COLOR_WHITE, COLOR_RED);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_PREFIX, COLOR_CYAN, -1);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_ADD_SUB, COLOR_GREEN, -1);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_WILDCARD, COLOR_MAGENTA, -1);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_ESCAPE, -1, -1);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_NUMBER, COLOR_BLUE, -1);
    this->_initColor(_COLOR_ID_SEARCH_INPUT_VIEW_ERROR, COLOR_WHITE, COLOR_RED);
}

void Stylist::_initColor(const int id, const int fg, const int bg) const
{
    auto ret = init_pair(id, fg, bg);

    assert(ret == OK);
    JACQUES_UNUSED(ret);
}

void Stylist::_attrsReset(WINDOW *window) const
{
    wattrset(window, 0);
}

void Stylist::_color(WINDOW *window, const int pair) const
{
    int ret = wcolor_set(window, pair, NULL);

    assert(ret == OK);
    JACQUES_UNUSED(ret);
}

void Stylist::_attrs(WINDOW *window, const int attrs) const
{
    int ret = wattron(window, attrs);

    assert(ret == OK);
    JACQUES_UNUSED(ret);
}

void Stylist::viewBorder(const View& view, const bool focused,
                         const bool emphasized) const
{
    this->_attrsReset(view);

    if (!focused) {
        this->_color(view, _COLOR_ID_VIEW_BORDER_BLURRED);
    } else {
        if (emphasized) {
            this->_color(view, _COLOR_ID_VIEW_BORDER_EMPHASIZED);
        } else {
            this->_color(view, _COLOR_ID_VIEW_BORDER_FOCUSED);
        }
    }

    this->_attrs(view, focused ? A_BOLD : A_DIM);
}

void Stylist::viewTitle(const View& view, const bool focused,
                        const bool emphasized) const
{
    this->viewBorder(view, focused, emphasized);
}

void Stylist::viewHasMore(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_VIEW_HAS_MORE);
}

void Stylist::tableViewTextCell(const View& view,
                                        const bool emphasized) const
{
    this->_attrsReset(view);

    if (emphasized) {
        this->_color(view, _COLOR_ID_TABLE_VIEW_TEXT_CELL_EMPHASIZED);
    } else {
        this->_color(view, _COLOR_ID_STD);
    }
}

void Stylist::tableViewNaCell(const View& view,
                                      const bool emphasized) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_STD);
    this->_attrs(view, A_DIM);

    if (emphasized) {
        this->_attrs(view, A_BOLD);
    }
}

void Stylist::tableViewBoolCell(const View& view, const bool value,
                                        const bool emphasized) const
{
    this->_attrsReset(view);
    this->_color(view, value ? _COLOR_ID_BOOL_YES : _COLOR_ID_BOOL_NO);

    if (emphasized) {
        this->_attrs(view, A_BOLD);
    }
}

void Stylist::tableViewHeader(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_STD);
    this->_attrs(view, A_REVERSE);
}

void Stylist::tableViewSelection(const View& view, const bool error) const
{
    this->tableViewSelectionSep(view, error);
    this->_attrs(view, A_BOLD);
}

void Stylist::tableViewSelectionSep(const View& view, const bool error) const
{
    this->_attrsReset(view);

    if (error) {
        this->_color(view, _COLOR_ID_TABLE_VIEW_SELECTION_ERROR);
    } else {
        this->_color(view, _COLOR_ID_TABLE_VIEW_SELECTION);
    }
}

void Stylist::tableViewSep(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_STD);
    this->_attrs(view, A_DIM);
}

void Stylist::textMore(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_TEXT_MORE);
    this->_attrs(view, A_BOLD);
}

void Stylist::tableViewWarningCell(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_TABLE_VIEW_WARNING_CELL);
    this->_attrs(view, A_BOLD);
}

void Stylist::tableViewErrorCell(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_TABLE_VIEW_ERROR_CELL);
    this->_attrs(view, A_BOLD);
}

void Stylist::std(WINDOW *window, const bool emphasized) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_STD);

    if (emphasized) {
        this->_attrs(window, A_BOLD);
    }
}

void Stylist::stdDim(const View& view) const
{
    this->std(view);
    this->_attrs(view, A_DIM);
}

void Stylist::stdHighlight(WINDOW *window) const
{
    this->_attrsReset(window);
    this->std(window);
    this->_attrs(window, A_REVERSE);
}

void Stylist::helpViewSection(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_HELP_VIEW_SECTION);
}

void Stylist::helpViewKey(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_HELP_VIEW_KEY);
}

void Stylist::statusViewStd(const View& view, const bool emphasized) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_STATUS_VIEW_STD);

    if (emphasized) {
        this->_attrs(view, A_BOLD);
    }
}

void Stylist::statusViewFilename(const View& view) const
{
    this->statusViewStd(view);
    this->_attrs(view, A_BOLD);
}

void Stylist::simpleInputViewBorder(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SIMPLE_INPUT_VIEW_BORDER);
}

void Stylist::packetIndexBuildProgressViewPath(const View& view,
                                               const bool filename) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_PACKET_INDEX_BUILD_PROGRESS_VIEW_PATH);

    if (filename) {
        this->_attrs(view, A_BOLD);
    }
}

void Stylist::packetIndexBuildProgressViewBar(const View& view,
                                              const bool on) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_STD);

    if (on) {
        this->_attrs(view, A_REVERSE | A_BOLD);
    } else {
        this->_attrs(view, A_DIM);
    }
}

void Stylist::error() const
{
    attrset(A_BOLD);
    color_set(_COLOR_ID_TABLE_VIEW_ERROR_CELL, NULL);
}

void Stylist::error(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_TABLE_VIEW_ERROR_CELL);
    this->_attrs(window, A_BOLD);
}

void Stylist::detailsViewSubtitle(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_DETAILS_VIEW_SUBTITLE);
    this->_attrs(view, A_BOLD);
}

void Stylist::detailsViewTypeInfo(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_DETAILS_VIEW_TYPE_INFO);
    this->_attrs(window, A_BOLD);
}

void Stylist::detailsViewDataTypeName(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_DETAILS_VIEW_DATA_TYPE_NAME);
    this->_attrs(window, A_BOLD);
}

void Stylist::detailsViewEnumDataTypeMemberName(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_NAME);
}

void Stylist::detailsViewEnumDataTypeMemberRange(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_RANGE);
}

void Stylist::detailsViewPropKey(WINDOW *window) const
{
    this->std(window, true);
}

void Stylist::detailsViewPropValue(WINDOW *window) const
{
    this->_attrsReset(window);
    this->_color(window, _COLOR_ID_DETAILS_VIEW_PROP_VALUE);
}

void Stylist::traceInfosViewSection(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_DETAILS_VIEW_SUBTITLE);
    this->_attrs(view, A_BOLD);
}

void Stylist::traceInfosViewPropKey(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_TRACE_INFOS_VIEW_PROP_VALUE);
}

void Stylist::traceInfosViewPropValue(const View& view) const
{
    this->std(view, false);
}

void Stylist::packetDecodingErrorDetailsView(const View& view,
                                             const bool emphasized) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_PACKET_DECODING_ERROR_DETAILS_VIEW);

    if (emphasized) {
        this->_attrs(view, A_BOLD);
    }
}

void Stylist::searchInputViewPrefix(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_PREFIX);
    this->_attrs(view, A_BOLD);
}

void Stylist::searchInputViewAddSub(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_ADD_SUB);
    this->_attrs(view, A_BOLD);
}

void Stylist::searchInputViewWildcard(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_WILDCARD);
    this->_attrs(view, A_BOLD);
}

void Stylist::searchInputViewEscape(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_ESCAPE);
    this->_attrs(view, A_BOLD);
}

void Stylist::searchInputViewNumber(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_NUMBER);
}

void Stylist::searchInputViewError(const View& view) const
{
    this->_attrsReset(view);
    this->_color(view, _COLOR_ID_SEARCH_INPUT_VIEW_ERROR);
    this->_attrs(view, A_BOLD);
}

} // namespace jacques
