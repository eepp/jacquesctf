/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <curses.h>

#include "stylist.hpp"
#include "views/view.hpp"
#include "utils.hpp"

namespace jacques {

Stylist::Stylist()
{
    this->_registerStyle(_StyleId::VIEW_BORDER_FOCUSED, -1, false, -1);
    this->_registerStyle(_StyleId::VIEW_BORDER_BLURRED, COLOR_BLACK, true, -1);
    this->_registerStyle(_StyleId::VIEW_BORDER_EMPHASIZED, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::VIEW_TITLE_FOCUSED, -1, false, -1);
    this->_registerStyle(_StyleId::VIEW_TITLE_BLURRED, -1, false, -1);
    this->_registerStyle(_StyleId::VIEW_TITLE_EMPHASIZED, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::VIEW_HAS_MORE, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::STD, -1, false, -1);
    this->_registerStyle(_StyleId::STD_DIM, COLOR_BLACK, true, -1);
    this->_registerStyle(_StyleId::STD_SEL, COLOR_CYAN, false, -1);
    this->_registerStyle(_StyleId::TABLE_VIEW_HEADER, COLOR_BLACK, false, COLOR_GREEN);
    this->_registerStyle(_StyleId::BOOL_YES, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::BOOL_NO, COLOR_MAGENTA, false, -1);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEL_ERROR, COLOR_RED, true, COLOR_CYAN);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEL_WARNING, COLOR_YELLOW, true, COLOR_CYAN);
    this->_registerStyle(_StyleId::TABLE_VIEW_TEXT_CELL_EMPHASIZED, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::TABLE_VIEW_TS_CELL_NS_PART, COLOR_CYAN, false, -1);
    this->_registerStyle(_StyleId::TEXT_MORE, COLOR_MAGENTA, true, -1);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEP, COLOR_WHITE, false, -1);
    this->_registerStyle(_StyleId::SECTION_TITLE, COLOR_CYAN, true, -1);
    this->_registerStyle(_StyleId::SUBSECTION_TITLE, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::HELP_VIEW_KEY, COLOR_MAGENTA, true, -1);
    this->_registerStyle(_StyleId::STATUS_VIEW_STD, COLOR_WHITE, false, COLOR_BLUE);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_STD, COLOR_WHITE, false, COLOR_MAGENTA);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_VAL, COLOR_YELLOW, true, COLOR_MAGENTA);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_ERROR, COLOR_RED, false, COLOR_WHITE);
    this->_registerStyle(_StyleId::TABLE_VIEW_WARNING_CELL, COLOR_YELLOW, true, -1);
    this->_registerStyle(_StyleId::TABLE_VIEW_ERROR_CELL, COLOR_RED, true, -1);
    this->_registerStyle(_StyleId::SIMPLE_INPUT_VIEW_BORDER, COLOR_BLACK, false, COLOR_GREEN);
    this->_registerStyle(_StyleId::PKT_INDEX_BUILD_PROGRESS_VIEW_PATH, COLOR_BLUE, false, -1);
    this->_registerStyle(_StyleId::DETAILS_VIEW_TYPE_INFO, COLOR_MAGENTA, true, -1);
    this->_registerStyle(_StyleId::DETAILS_VIEW_DT_NAME, COLOR_WHITE, false, COLOR_BLUE);
    this->_registerStyle(_StyleId::DETAILS_VIEW_DT_EXTRA, COLOR_WHITE, false, COLOR_GREEN);
    this->_registerStyle(_StyleId::DETAILS_VIEW_ENUM_TYPE_MEMBER_NAME, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::DETAILS_VIEW_ENUM_TYPE_MEMBER_RANGE, COLOR_CYAN, false, -1);
    this->_registerStyle(_StyleId::DETAILS_VIEW_PROP_KEY, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::DETAILS_VIEW_PROP_VAL, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::TRACE_INFO_VIEW_PROP_VAL, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::PKT_DECODING_ERROR_DETAILS_VIEW, COLOR_WHITE, false, COLOR_RED);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_PREFIX, COLOR_CYAN, false, -1);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ADD_SUB, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_WILDCARD, COLOR_MAGENTA, true, -1);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ESCAPE, -1, false, -1);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_NUMBER, COLOR_BLUE, false, -1);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ERROR, COLOR_WHITE, false, COLOR_RED);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_SEL_PREV, COLOR_MAGENTA, true, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_SEL_NEXT, COLOR_GREEN, false, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_OFFSET, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_OFFSET_CUR, COLOR_YELLOW, true, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_PADDING, COLOR_BLUE, false, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_ER_FIRST_PKT_REGION, COLOR_YELLOW, false, -1);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_1, COLOR_BLACK, false, COLOR_YELLOW);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_2, COLOR_BLACK, false, COLOR_GREEN);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_3, COLOR_BLACK, false, COLOR_BLUE);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_4, COLOR_BLACK, false, COLOR_MAGENTA);
}

void Stylist::_registerStyle(const _StyleId id, int fg, const bool fgIsBright, const int bg)
{
    const auto intId = static_cast<unsigned int>(id);

    assert(fg < 8);

    if (fgIsBright && this->_supportsBrightColors()) {
        fg += 8;
    }

    assert(bg < 8);

    const auto ret = init_pair(intId, fg, bg);

    assert(ret == OK);
    static_cast<void>(ret);

    if (_styles.size() <= intId) {
        _styles.resize(intId + 1);
    }

    auto& style = _styles[intId];

    style.colorPair = intId;
    style.fgIsBright = fgIsBright;
}

void Stylist::_applyStyle(WINDOW * const window, const _StyleId id, const int extraAttrs) const
{
    const auto intId = static_cast<unsigned int>(id);

    assert(intId < _styles.size());

    auto ret = wattrset(window, 0);

    assert(ret == OK);
    static_cast<void>(ret);

    const auto& style = _styles[intId];

    ret = wcolor_set(window, style.colorPair, NULL);
    assert(ret == OK);
    static_cast<void>(ret);

    auto attrs = extraAttrs;

    if (style.fgIsBright && !this->_supportsBrightColors()) {
        /*
         * Most terminals which don't support SGR colors 8-15 (bright)
         * will still show bright colors with the bold attribute,
         * although with a bold-supporting font, it might be bright AND
         * bold. It's a tradeoff.
         */
        attrs |= A_BOLD;
    }

    if (attrs != 0) {
        ret = wattron(window, attrs);
        assert(ret == OK);
        static_cast<void>(ret);
    }
}

void Stylist::viewBorder(const View& view, const bool focused, const bool emphasized) const
{
    if (!focused) {
        this->_applyStyle(view, _StyleId::VIEW_BORDER_BLURRED);
    } else {
        if (emphasized) {
            this->_applyStyle(view, _StyleId::VIEW_BORDER_EMPHASIZED, A_BOLD);
        } else {
            this->_applyStyle(view, _StyleId::VIEW_BORDER_FOCUSED);
        }
    }
}

void Stylist::viewTitle(const View& view, const bool focused, const bool emphasized) const
{
    this->viewBorder(view, focused, emphasized);
}

void Stylist::viewHasMore(const View& view) const
{
    this->_applyStyle(view, _StyleId::VIEW_HAS_MORE);
}

void Stylist::tableViewTextCell(const View& view, const bool emphasized) const
{
    if (emphasized) {
        this->_applyStyle(view, _StyleId::TABLE_VIEW_TEXT_CELL_EMPHASIZED, A_BOLD);
    } else {
        this->_applyStyle(view, _StyleId::STD);
    }
}

void Stylist::tableViewTsCellNsPart(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::TABLE_VIEW_TS_CELL_NS_PART, emphasized ? A_BOLD : 0);
}

void Stylist::tableViewNaCell(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::STD_DIM, emphasized ? A_BOLD : 0);
}

void Stylist::tableViewBoolCell(const View& view, const bool val, const bool emphasized) const
{
    this->_applyStyle(view, val ? _StyleId::BOOL_YES : _StyleId::BOOL_NO, emphasized ? A_BOLD : 0);
}

void Stylist::tableViewHeader(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD, A_REVERSE);
}

void Stylist::tableViewSel(const View& view, const TableViewCellStyle style) const
{
    this->tableViewSelSep(view, style);
}

void Stylist::tableViewSelSep(const View& view, const TableViewCellStyle style) const
{
    switch (style) {
    case TableViewCellStyle::NORMAL:
        this->stdSel(view);
        break;

    case TableViewCellStyle::WARNING:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_SEL_WARNING, A_BOLD);
        break;

    case TableViewCellStyle::ERROR:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_SEL_ERROR, A_BOLD);
        break;
    }
}

void Stylist::tableViewSep(const View& view) const
{
    this->_applyStyle(view, _StyleId::TABLE_VIEW_SEP);
}

void Stylist::textMore(const View& view) const
{
    this->_applyStyle(view, _StyleId::TEXT_MORE, A_BOLD);
}

void Stylist::tableViewCell(const View& view, const TableViewCellStyle style) const
{
    switch (style) {
    case TableViewCellStyle::NORMAL:
        this->std(view);
        break;

    case TableViewCellStyle::WARNING:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_WARNING_CELL, A_BOLD);
        break;

    case TableViewCellStyle::ERROR:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_ERROR_CELL, A_BOLD);
        break;
    }
}

void Stylist::std(WINDOW * const window, const bool emphasized) const
{
    this->_applyStyle(window, _StyleId::STD, emphasized ? A_BOLD : 0);
}

void Stylist::stdDim(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD_DIM);
}

void Stylist::stdSel(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::STD_SEL, A_BOLD | A_REVERSE);
}

void Stylist::sectionTitle(const View& view) const
{
    this->_applyStyle(view, _StyleId::SECTION_TITLE, A_UNDERLINE | A_BOLD);
}

void Stylist::subsectionTitle(const View& view) const
{
    this->_applyStyle(view, _StyleId::SUBSECTION_TITLE, A_UNDERLINE);
}

void Stylist::helpViewKey(const View& view) const
{
    this->_applyStyle(view, _StyleId::HELP_VIEW_KEY);
}

void Stylist::statusViewStd(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::STATUS_VIEW_STD, emphasized ? A_BOLD : 0);
}

void Stylist::statusViewFilename(const View& view) const
{
    this->_applyStyle(view, _StyleId::STATUS_VIEW_STD, A_BOLD);
}

void Stylist::pktRegionInfoViewStd(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_STD, emphasized ? A_BOLD : 0);
}

void Stylist::pktRegionInfoViewVal(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_VAL, A_BOLD);
}

void Stylist::pktRegionInfoViewError(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_ERROR, A_BOLD);
}

void Stylist::simpleInputViewBorder(const View& view) const
{
    this->_applyStyle(view, _StyleId::SIMPLE_INPUT_VIEW_BORDER);
}

void Stylist::pktIndexBuildProgressViewPath(const View& view, const bool filename) const
{
    this->_applyStyle(view, _StyleId::PKT_INDEX_BUILD_PROGRESS_VIEW_PATH, filename ? A_BOLD : 0);
}

void Stylist::pktIndexBuildProgressViewBar(const View& view, const bool on) const
{
    if (on) {
        this->_applyStyle(view, _StyleId::STD, A_REVERSE | A_BOLD);
    } else {
        this->stdDim(view);
    }
}

void Stylist::error() const
{
    attrset(A_BOLD);
    color_set(static_cast<int>(_StyleId::TABLE_VIEW_ERROR_CELL), NULL);
}

void Stylist::error(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::TABLE_VIEW_ERROR_CELL, A_BOLD);
}

void Stylist::detailsViewTypeInfo(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::DETAILS_VIEW_TYPE_INFO);
}

void Stylist::detailsViewDtName(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::DETAILS_VIEW_DT_NAME, A_BOLD);
}

void Stylist::detailsViewDtExtra(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::DETAILS_VIEW_DT_EXTRA, A_BOLD);
}

void Stylist::detailsViewEnumTypeMappingName(WINDOW * const window) const
{
    this->_applyStyle(window,
                      _StyleId::DETAILS_VIEW_ENUM_TYPE_MEMBER_NAME);
}

void Stylist::detailsViewIntRanges(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::DETAILS_VIEW_ENUM_TYPE_MEMBER_RANGE);
}

void Stylist::detailsViewPropKey(WINDOW * const window) const
{
    this->std(window, true);
}

void Stylist::detailsViewPropVal(WINDOW * const window) const
{
    this->_applyStyle(window, _StyleId::DETAILS_VIEW_PROP_VAL);
}

void Stylist::traceInfoViewPropKey(const View& view) const
{
    this->_applyStyle(view, _StyleId::TRACE_INFO_VIEW_PROP_VAL);
}

void Stylist::traceInfoViewPropVal(const View& view) const
{
    this->std(view, false);
}

void Stylist::pktDecodingErrorDetailsView(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::PKT_DECODING_ERROR_DETAILS_VIEW, emphasized ? A_BOLD : 0);
}

void Stylist::searchInputViewPrefix(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_PREFIX, A_BOLD);
}

void Stylist::searchInputViewAddSub(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ADD_SUB, A_BOLD);
}

void Stylist::searchInputViewWildcard(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_WILDCARD, A_BOLD);
}

void Stylist::searchInputViewEscape(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ESCAPE, A_BOLD);
}

void Stylist::searchInputViewNumber(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_NUMBER);
}

void Stylist::searchInputViewError(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ERROR, A_BOLD);
}

void Stylist::pktDataViewSel(const View& view, const PktDataViewSelType& selType) const
{
    _StyleId styleId;

    switch (selType) {
    case PktDataViewSelType::PREV:
        styleId = _StyleId::PKT_DATA_VIEW_SEL_PREV;
        break;

    case PktDataViewSelType::CUR:
        this->stdSel(view);
        return;

    case PktDataViewSelType::NEXT:
        styleId = _StyleId::PKT_DATA_VIEW_SEL_NEXT;
        break;

    default:
        std::abort();
    }

    this->_applyStyle(view, styleId, A_BOLD);
}

void Stylist::pktDataViewAuxSel(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD, A_BOLD | A_REVERSE);
}

void Stylist::pktDataViewOffset(const View& view, const bool sel) const
{
    if (sel) {
        this->_applyStyle(view, _StyleId::PKT_DATA_VIEW_OFFSET_CUR, A_BOLD);
    } else {
        this->_applyStyle(view, _StyleId::PKT_DATA_VIEW_OFFSET);
    }
}

void Stylist::pktDataViewPadding(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_DATA_VIEW_PADDING);
}

void Stylist::pktDataViewBookmark(const View& view, const unsigned int id) const
{
    assert(id <= 3);

    _StyleId styleId;

    switch (id) {
    case 0:
        styleId = _StyleId::PKT_DATA_VIEW_BOOKMARK_1;
        break;

    case 1:
        styleId = _StyleId::PKT_DATA_VIEW_BOOKMARK_2;
        break;

    case 2:
        styleId = _StyleId::PKT_DATA_VIEW_BOOKMARK_3;
        break;

    case 3:
        styleId = _StyleId::PKT_DATA_VIEW_BOOKMARK_4;
        break;

    default:
        std::abort();
    }

    this->_applyStyle(view, styleId);
}

void Stylist::pktDataViewErFirstPktRegion(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_DATA_VIEW_ER_FIRST_PKT_REGION);
}

} // namespace jacques
