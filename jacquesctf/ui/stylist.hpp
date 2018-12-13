/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_STYLIST_HPP
#define _JACQUES_STYLIST_HPP

#include <curses.h>
#include <boost/core/noncopyable.hpp>

#include "view.hpp"

namespace jacques {

class Stylist :
    boost::noncopyable
{
public:
    enum class PacketDataViewSelectionType
    {
        PREVIOUS,
        CURRENT,
        NEXT,
    };

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

    void stdSelection(WINDOW *window) const;

    void stdSelection(const View& view) const
    {
        this->stdSelection(view._window());
    }

    void helpViewSection(const View& view) const;
    void helpViewSubSection(const View& view) const;
    void helpViewKey(const View& view) const;
    void statusViewStd(const View& view, bool emphasized = false) const;
    void statusViewFilename(const View& view) const;
    void packetRegionInfoViewStd(const View& view, bool emphasized = false) const;
    void packetRegionInfoViewValue(const View& view) const;
    void packetRegionInfoViewError(const View& view) const;
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
    void packetDataViewOffset(const View& view, bool selected = false) const;
    void packetDataViewPadding(const View& view) const;
    void packetDataViewEventRecordFirstPacketRegion(const View& view) const;
    void packetDataViewBookmark(const View& view, unsigned int id) const;
    void packetDataViewSelection(const View& view,
                                 const PacketDataViewSelectionType& selectionType) const;
    void packetDataViewAuxSelection(const View& view,
                                    const PacketDataViewSelectionType& selectionType) const;

private:
    struct _Style {
        int colorPair;
        bool fgIsBright;
    };

private:
    enum class _StyleId {
        VIEW_BORDER_FOCUSED = 1,
        VIEW_BORDER_EMPHASIZED,
        VIEW_BORDER_BLURRED,
        VIEW_TITLE_FOCUSED,
        VIEW_TITLE_EMPHASIZED,
        VIEW_TITLE_BLURRED,
        VIEW_HAS_MORE,
        STD,
        STD_DIM,
        STD_SELECTION,
        TABLE_VIEW_HEADER,
        BOOL_YES,
        BOOL_NO,
        TABLE_VIEW_SELECTION_ERROR,
        TEXT_MORE,
        TABLE_VIEW_SEP,
        TABLE_VIEW_WARNING_CELL,
        TABLE_VIEW_ERROR_CELL,
        TABLE_VIEW_TEXT_CELL_EMPHASIZED,
        TABLE_VIEW_TS_CELL_NS_PART,
        HELP_VIEW_SECTION,
        HELP_VIEW_SUB_SECTION,
        HELP_VIEW_KEY,
        STATUS_VIEW_STD,
        PACKET_REGION_INFO_VIEW_STD,
        PACKET_REGION_INFO_VIEW_VALUE,
        PACKET_REGION_INFO_VIEW_ERROR,
        SIMPLE_INPUT_VIEW_BORDER,
        PACKET_INDEX_BUILD_PROGRESS_VIEW_PATH,
        DETAILS_VIEW_SUBTITLE,
        DETAILS_VIEW_TYPE_INFO,
        DETAILS_VIEW_DATA_TYPE_NAME,
        DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_NAME,
        DETAILS_VIEW_ENUM_DATA_TYPE_MEMBER_RANGE,
        DETAILS_VIEW_PROP_KEY,
        DETAILS_VIEW_PROP_VALUE,
        TRACE_INFO_VIEW_PROP_VALUE,
        PACKET_DECODING_ERROR_DETAILS_VIEW,
        SEARCH_INPUT_VIEW_PREFIX,
        SEARCH_INPUT_VIEW_ADD_SUB,
        SEARCH_INPUT_VIEW_WILDCARD,
        SEARCH_INPUT_VIEW_ESCAPE,
        SEARCH_INPUT_VIEW_NUMBER,
        SEARCH_INPUT_VIEW_ERROR,
        PACKET_DATA_VIEW_SELECTION_PREVIOUS,
        PACKET_DATA_VIEW_SELECTION_NEXT,
        PACKET_DATA_VIEW_OFFSET,
        PACKET_DATA_VIEW_OFFSET_CURRENT,
        PACKET_DATA_VIEW_PADDING,
        PACKET_DATA_VIEW_EVENT_RECORD_FIRST_PACKET_REGION,
        PACKET_DATA_VIEW_BOOKMARK_1,
        PACKET_DATA_VIEW_BOOKMARK_2,
        PACKET_DATA_VIEW_BOOKMARK_3,
        PACKET_DATA_VIEW_BOOKMARK_4,
    };

private:
    bool _supportsBrightColors() const
    {
        return COLORS >= 16;
    }

    void _initColor(int id, int fg, int bg) const;
    void _registerStyle(_StyleId id, int fg, bool fgIsBright, int bg);
    void _applyStyle(WINDOW *window, _StyleId id, int extraAttrs = 0) const;

    void _applyStyle(const View& view, _StyleId id, int extraAttrs = 0) const
    {
        this->_applyStyle(view._window(), id, extraAttrs);
    }

    void _color(WINDOW *window, int pair) const;

    void _color(const View& view, const int pair) const
    {
        this->_color(view._window(), pair);
    }

private:
    std::vector<_Style> _styles;
};

} // namespace jacques

#endif // _JACQUES_STYLIST_HPP
