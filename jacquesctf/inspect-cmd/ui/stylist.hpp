/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP
#define _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP

#include <cstdint>
#include <vector>

#include "color.hpp"
#include "views/view.hpp"

struct ncplane;

namespace jacques {

class Stylist final
{
public:
    enum class PktDataViewSelType
    {
        PREV,
        CUR,
        NEXT,
    };

    enum class TableViewCellStyle
    {
        NORMAL,
        WARNING,
        ERROR,
    };

public:
    Stylist();

    Stylist(const Stylist&) = delete;
    Stylist& operator=(const Stylist&) = delete;

    void viewBorder(const View& view, bool focused, bool emphasized) const;
    void viewTitle(const View& view, bool focused, bool emphasized) const;
    void viewHasMore(const View& view) const;
    void tableViewTsCellNsPart(const View& view, bool emphasized) const;
    void tableViewTextCell(const View& view, bool emphasized) const;
    void tableViewNaCell(const View& view, bool emphasized) const;
    void tableViewBoolCell(const View& view, bool value, bool emphasized) const;
    void tableViewHeader(const View& view) const;
    void tableViewSel(const View& view, TableViewCellStyle style = TableViewCellStyle::NORMAL) const;
    void tableViewSelSep(const View& view, TableViewCellStyle style = TableViewCellStyle::NORMAL) const;
    void tableViewSep(const View& view) const;
    void tableViewCell(const View& view, TableViewCellStyle style = TableViewCellStyle::NORMAL) const;
    void textMore(const View& view) const;
    void stdDim(const View& view) const;
    void std(ncplane *plane, bool emphasized = false) const;

    void std(const View& view, const bool emphasized = false) const
    {
        this->std(view._plane(), emphasized);
    }

    void stdSel(ncplane *plane) const;

    void stdSel(const View& view) const
    {
        this->stdSel(view._plane());
    }

    void sectionTitle(const View& view) const;
    void subsectionTitle(const View& view) const;
    void helpViewKey(const View& view) const;
    void statusViewStd(const View& view, bool emphasized = false) const;
    void statusViewFilename(const View& view) const;
    void pktRegionInfoViewStd(const View& view, bool emphasized = false) const;
    void pktRegionInfoViewVal(const View& view) const;
    void pktRegionInfoViewError(const View& view) const;
    void simpleInputViewBorder(const View& view) const;
    void pktIndexBuildProgressViewPath(const View& view, bool filename) const;
    void pktIndexBuildProgressViewBar(const View& view, bool on) const;
    void detailsViewTypeInfo(ncplane *plane) const;
    void detailsViewDtName(ncplane *plane) const;
    void detailsViewDtExtra(ncplane *plane) const;
    void detailsViewMappingOrFlagName(ncplane *plane) const;
    void detailsViewMappingOrFlagRange(ncplane *plane) const;
    void detailsViewPropKey(ncplane *plane) const;
    void detailsViewPropVal(ncplane *plane) const;
    void traceInfoViewPropKey(const View& view) const;
    void traceInfoViewPropVal(const View& view) const;
    void pktDecodingErrorDetailsView(const View& view, bool emphasized = false) const;
    void error(ncplane *plane) const;

    void error(const View& view) const
    {
        this->error(view._plane());
    }

    void searchInputViewPrefix(const View& view) const;
    void searchInputViewAddSub(const View& view) const;
    void searchInputViewWildcard(const View& view) const;
    void searchInputViewEscape(const View& view) const;
    void searchInputViewNumber(const View& view) const;
    void searchInputViewError(const View& view) const;
    void pktDataViewOffset(const View& view, bool sel = false) const;
    void pktDataViewPadding(const View& view) const;
    void pktDataViewErFirstPktRegion(const View& view) const;
    void pktDataViewBookmark(const View& view, unsigned int id) const;
    void pktDataViewSel(const View& view, const PktDataViewSelType& selType) const;
    void pktDataViewAuxSel(const View& view) const;

private:
    struct _Style {
        // notcurses channels (foreground and background colors)
        std::uint64_t channels;

        // reversed version of `channels`
        std::uint64_t revChannels;
    };

    // extra attributes (bitmask) to apply with a style
    static constexpr unsigned int _ATTR_BOLD = 1U << 0;
    static constexpr unsigned int _ATTR_UNDERLINE = 1U << 1;
    static constexpr unsigned int _ATTR_REVERSE = 1U << 2;

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
        STD_SEL,
        TABLE_VIEW_HEADER,
        BOOL_YES,
        BOOL_NO,
        TABLE_VIEW_SEL_WARNING,
        TABLE_VIEW_SEL_ERROR,
        TEXT_MORE,
        TABLE_VIEW_SEP,
        TABLE_VIEW_WARNING_CELL,
        TABLE_VIEW_ERROR_CELL,
        TABLE_VIEW_TEXT_CELL_EMPHASIZED,
        TABLE_VIEW_TS_CELL_NS_PART,
        SECTION_TITLE,
        SUBSECTION_TITLE,
        HELP_VIEW_KEY,
        STATUS_VIEW_STD,
        PKT_REGION_INFO_VIEW_STD,
        PKT_REGION_INFO_VIEW_EMPHASIZED,
        PKT_REGION_INFO_VIEW_VAL,
        PKT_REGION_INFO_VIEW_ERROR,
        SIMPLE_INPUT_VIEW_BORDER,
        PKT_INDEX_BUILD_PROGRESS_VIEW_PATH,
        DETAILS_VIEW_TYPE_INFO,
        DETAILS_VIEW_DT_NAME,
        DETAILS_VIEW_DT_EXTRA,
        DETAILS_VIEW_MAPPING_OR_FLAG_NAME,
        DETAILS_VIEW_MAPPING_OR_FLAG_RANGE,
        DETAILS_VIEW_PROP_KEY,
        DETAILS_VIEW_PROP_VAL,
        TRACE_INFO_VIEW_PROP_VAL,
        PKT_DECODING_ERROR_DETAILS_VIEW,
        SEARCH_INPUT_VIEW_PREFIX,
        SEARCH_INPUT_VIEW_ADD_SUB,
        SEARCH_INPUT_VIEW_WILDCARD,
        SEARCH_INPUT_VIEW_ESCAPE,
        SEARCH_INPUT_VIEW_NUMBER,
        SEARCH_INPUT_VIEW_ERROR,
        PKT_DATA_VIEW_SEL_PREV,
        PKT_DATA_VIEW_SEL_NEXT,
        PKT_DATA_VIEW_OFFSET,
        PKT_DATA_VIEW_OFFSET_CUR,
        PKT_DATA_VIEW_PADDING,
        PKT_DATA_VIEW_ER_FIRST_PKT_REGION,
        PKT_DATA_VIEW_BOOKMARK_1,
        PKT_DATA_VIEW_BOOKMARK_2,
        PKT_DATA_VIEW_BOOKMARK_3,
        PKT_DATA_VIEW_BOOKMARK_4,
    };

private:
    void _registerStyle(_StyleId id, const Color& fg, const Color& bg);
    void _applyStyle(ncplane *plane, _StyleId id, unsigned int extraAttrs = 0) const;

    void _applyStyle(const View& view, const _StyleId id, const unsigned int extraAttrs = 0) const
    {
        this->_applyStyle(view._plane(), id, extraAttrs);
    }

private:
    std::vector<_Style> _styles;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP
