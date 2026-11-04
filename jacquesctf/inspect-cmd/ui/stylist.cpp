/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <notcurses/notcurses.h>

#include "stylist.hpp"
#include "term.hpp"
#include "views/view.hpp"
#include "utils.hpp"

namespace jacques {

namespace {
namespace fbcolors {

/*
 * Colors to use when the terminal doesn't report its own (Tango basic
 * colors for the palette).
 */
constexpr Color fg {0xd3, 0xe3, 0xe5};
constexpr Color bg {0x08, 0x1a, 0x1f};
constexpr Color red {0xcc, 0x00, 0x00};
constexpr Color green {0x4e, 0x9a, 0x06};
constexpr Color yellow {0xc4, 0xa0, 0x00};
constexpr Color blue {0x34, 0x65, 0xa4};
constexpr Color magenta {0x75, 0x50, 0x7b};
constexpr Color cyan {0x06, 0x98, 0x9a};
constexpr Color brightRed {0xef, 0x29, 0x29};
constexpr Color brightYellow {0xfc, 0xe9, 0x4f};
constexpr Color brightBlue {0x72, 0x9f, 0xcf};
constexpr Color brightMagenta {0xad, 0x7f, 0xa8};
constexpr Color brightCyan {0x34, 0xe2, 0xe2};

} // namespace fbcolors

/*
 * Returns notcurses channels having the foreground color `fg` and the
 * background color `bg`.
 */
std::uint64_t ncChannels(const Color& fg, const Color& bg) noexcept
{
    std::uint64_t channels = 0;

    ncchannels_set_fg_rgb(&channels, fg.ncRgb());
    ncchannels_set_bg_rgb(&channels, bg.ncRgb());
    return channels;
}

} // namespace

Stylist::Stylist()
{
    const auto palColor = [](const unsigned int index, const Color& fallback) {
        return term::paletteColor(index).value_or(fallback);
    };

    const auto fg = term::defaultFgColor().value_or(fbcolors::fg);
    const auto bg = term::defaultBgColor().value_or(fbcolors::bg);
    const auto red = palColor(1, fbcolors::red);
    const auto green = palColor(2, fbcolors::green);
    const auto yellow = palColor(3, fbcolors::yellow);
    const auto blue = palColor(4, fbcolors::blue);
    const auto magenta = palColor(5, fbcolors::magenta);
    const auto cyan = palColor(6, fbcolors::cyan);
    const auto brightRed = palColor(9, fbcolors::brightRed);
    const auto brightYellow = palColor(11, fbcolors::brightYellow);
    const auto brightBlue = palColor(12, fbcolors::brightBlue);
    const auto brightMagenta = palColor(13, fbcolors::brightMagenta);
    const auto brightCyan = palColor(14, fbcolors::brightCyan);

    /*
     * Colors mixed from the ones above.
     */
    const auto dimFg = fg.mix(bg, .4);
    const auto faintFg = fg.mix(bg, .65);
    const auto selBg = cyan.mix(bg, .3);
    const auto darkBlueBg = blue.mix(bg, .5);
    const auto pktRegionInfoBg = magenta.mix(bg, .4);
    const auto pktRegionInfoFg = fg.mix(pktRegionInfoBg, .3);
    const auto darkGreenBg = green.mix(bg, .5);
    const auto errorBg = red.mix(bg, .4);
    const auto offsetFg = yellow.mix(bg, .15);

    this->_registerStyle(_StyleId::VIEW_BORDER_FOCUSED, fg, bg);
    this->_registerStyle(_StyleId::VIEW_BORDER_BLURRED, faintFg, bg);
    this->_registerStyle(_StyleId::VIEW_BORDER_EMPHASIZED, yellow, bg);
    this->_registerStyle(_StyleId::VIEW_TITLE_FOCUSED, fg, bg);
    this->_registerStyle(_StyleId::VIEW_TITLE_BLURRED, dimFg, bg);
    this->_registerStyle(_StyleId::VIEW_TITLE_EMPHASIZED, yellow, bg);
    this->_registerStyle(_StyleId::VIEW_HAS_MORE, yellow, bg);
    this->_registerStyle(_StyleId::STD, fg, bg);
    this->_registerStyle(_StyleId::STD_DIM, dimFg, bg);
    this->_registerStyle(_StyleId::STD_SEL, fg, selBg);
    this->_registerStyle(_StyleId::TABLE_VIEW_HEADER, fg, darkGreenBg);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEL_ERROR, fg, errorBg);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEL_WARNING, brightYellow, selBg);
    this->_registerStyle(_StyleId::TABLE_VIEW_TEXT_CELL_EMPHASIZED, yellow, bg);
    this->_registerStyle(_StyleId::TABLE_VIEW_TS_CELL_NS_PART, cyan, bg);
    this->_registerStyle(_StyleId::TEXT_MORE, brightMagenta, bg);
    this->_registerStyle(_StyleId::TABLE_VIEW_SEP, faintFg, bg);
    this->_registerStyle(_StyleId::SECTION_TITLE, brightCyan, bg);
    this->_registerStyle(_StyleId::SUBSECTION_TITLE, green, bg);
    this->_registerStyle(_StyleId::HELP_VIEW_KEY, brightMagenta, bg);
    this->_registerStyle(_StyleId::STATUS_VIEW_STD, fg, darkBlueBg);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_STD, pktRegionInfoFg, pktRegionInfoBg);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_EMPHASIZED, fg, pktRegionInfoBg);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_VAL, brightYellow, pktRegionInfoBg);
    this->_registerStyle(_StyleId::PKT_REGION_INFO_VIEW_ERROR, red, fg);
    this->_registerStyle(_StyleId::TABLE_VIEW_WARNING_CELL, brightYellow, bg);
    this->_registerStyle(_StyleId::TABLE_VIEW_ERROR_CELL, brightRed, bg);
    this->_registerStyle(_StyleId::SIMPLE_INPUT_VIEW_BORDER, bg, green);
    this->_registerStyle(_StyleId::PKT_INDEX_BUILD_PROGRESS_VIEW_PATH, brightBlue, bg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_TYPE_INFO, brightMagenta, bg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_DT_NAME, fg, darkBlueBg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_DT_EXTRA, fg, darkGreenBg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_MAPPING_OR_FLAG_NAME, yellow, bg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_MAPPING_OR_FLAG_RANGE, cyan, bg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_PROP_KEY, dimFg, bg);
    this->_registerStyle(_StyleId::DETAILS_VIEW_PROP_VAL, green, bg);
    this->_registerStyle(_StyleId::TRACE_INFO_VIEW_PROP_VAL, green, bg);
    this->_registerStyle(_StyleId::PKT_DECODING_ERROR_DETAILS_VIEW, fg, errorBg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_PREFIX, cyan, bg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ADD_SUB, green, bg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_WILDCARD, brightMagenta, bg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ESCAPE, fg, bg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_NUMBER, brightBlue, bg);
    this->_registerStyle(_StyleId::SEARCH_INPUT_VIEW_ERROR, fg, errorBg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_SEL_PREV, brightMagenta, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_SEL_NEXT, green, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_OFFSET, offsetFg, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_OFFSET_CUR, brightYellow, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_PADDING, blue, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_ER_FIRST_PKT_REGION, yellow, bg);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_1, bg, yellow);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_2, bg, green);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_3, fg, blue);
    this->_registerStyle(_StyleId::PKT_DATA_VIEW_BOOKMARK_4, fg, magenta);
}

void Stylist::_registerStyle(const _StyleId id, const Color& fg, const Color& bg)
{
    const auto intId = static_cast<unsigned int>(id);

    if (_styles.size() <= intId) {
        _styles.resize(intId + 1);
    }

    auto& style = _styles[intId];

    style.channels = ncChannels(fg, bg);

    // notcurses doesn't support reverse video: swap the colors ourselves
    style.revChannels = ncChannels(bg, fg);
}

void Stylist::_applyStyle(ncplane * const plane, const _StyleId id,
                          const unsigned int extraAttrs) const
{
    const auto intId = static_cast<unsigned int>(id);

    assert(intId < _styles.size());

    const auto& style = _styles[intId];

    ncplane_set_channels(plane, (extraAttrs & _ATTR_REVERSE) ? style.revChannels : style.channels);

    unsigned int ncStyles = 0;

    if (extraAttrs & _ATTR_BOLD) {
        /*
         * With emulated reverse video, the foreground color is
         * typically the default background color (black, for example).
         *
         * A terminal which doesn't support RGB colors gets palette
         * colors instead (notcurses quantizes them), and many terminals
         * show a bold foreground with the bright version of its palette
         * color: black would become gray and hard to read. Therefore
         * don't make such text bold.
         */
        if (!(extraAttrs & _ATTR_REVERSE)) {
            ncStyles |= NCSTYLE_BOLD;
        }
    }

    if (extraAttrs & _ATTR_UNDERLINE) {
        ncStyles |= NCSTYLE_UNDERLINE;
    }

    ncplane_set_styles(plane, ncStyles);
}

void Stylist::viewBorder(const View& view, const bool focused, const bool emphasized) const
{
    if (!focused) {
        this->_applyStyle(view, _StyleId::VIEW_BORDER_BLURRED);
    } else {
        if (emphasized) {
            this->_applyStyle(view, _StyleId::VIEW_BORDER_EMPHASIZED, _ATTR_BOLD);
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
        this->_applyStyle(view, _StyleId::TABLE_VIEW_TEXT_CELL_EMPHASIZED, _ATTR_BOLD);
    } else {
        this->_applyStyle(view, _StyleId::STD);
    }
}

void Stylist::tableViewTsCellNsPart(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::TABLE_VIEW_TS_CELL_NS_PART, emphasized ? _ATTR_BOLD : 0);
}

void Stylist::tableViewNaCell(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::STD_DIM, emphasized ? _ATTR_BOLD : 0);
}

void Stylist::tableViewHeader(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD, _ATTR_REVERSE);
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
        this->_applyStyle(view, _StyleId::TABLE_VIEW_SEL_WARNING, _ATTR_BOLD);
        break;

    case TableViewCellStyle::ERROR:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_SEL_ERROR, _ATTR_BOLD);
        break;
    }
}

void Stylist::tableViewSep(const View& view) const
{
    this->_applyStyle(view, _StyleId::TABLE_VIEW_SEP);
}

void Stylist::textMore(const View& view) const
{
    this->_applyStyle(view, _StyleId::TEXT_MORE, _ATTR_BOLD);
}

void Stylist::tableViewCell(const View& view, const TableViewCellStyle style) const
{
    switch (style) {
    case TableViewCellStyle::NORMAL:
        this->std(view);
        break;

    case TableViewCellStyle::WARNING:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_WARNING_CELL, _ATTR_BOLD);
        break;

    case TableViewCellStyle::ERROR:
        this->_applyStyle(view, _StyleId::TABLE_VIEW_ERROR_CELL, _ATTR_BOLD);
        break;
    }
}

void Stylist::std(ncplane * const plane, const bool emphasized) const
{
    this->_applyStyle(plane, _StyleId::STD, emphasized ? _ATTR_BOLD : 0);
}

void Stylist::stdDim(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD_DIM);
}

void Stylist::stdSel(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::STD_SEL, _ATTR_BOLD);
}

void Stylist::sectionTitle(const View& view) const
{
    this->_applyStyle(view, _StyleId::SECTION_TITLE, _ATTR_UNDERLINE | _ATTR_BOLD);
}

void Stylist::subsectionTitle(const View& view) const
{
    this->_applyStyle(view, _StyleId::SUBSECTION_TITLE, _ATTR_UNDERLINE);
}

void Stylist::helpViewKey(const View& view) const
{
    this->_applyStyle(view, _StyleId::HELP_VIEW_KEY);
}

void Stylist::statusViewStd(const View& view, const bool emphasized) const
{
    this->_applyStyle(view, _StyleId::STATUS_VIEW_STD, emphasized ? _ATTR_BOLD : 0);
}

void Stylist::statusViewFilename(const View& view) const
{
    this->_applyStyle(view, _StyleId::STATUS_VIEW_STD, _ATTR_BOLD);
}

void Stylist::pktRegionInfoViewStd(const View& view, const bool emphasized) const
{
    if (emphasized) {
        this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_EMPHASIZED, _ATTR_BOLD);
    } else {
        this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_STD);
    }
}

void Stylist::pktRegionInfoViewVal(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_VAL, _ATTR_BOLD);
}

void Stylist::pktRegionInfoViewError(const View& view) const
{
    this->_applyStyle(view, _StyleId::PKT_REGION_INFO_VIEW_ERROR, _ATTR_BOLD);
}

void Stylist::simpleInputViewBorder(const View& view) const
{
    this->_applyStyle(view, _StyleId::SIMPLE_INPUT_VIEW_BORDER);
}

void Stylist::pktIndexBuildProgressViewPath(const View& view, const bool filename) const
{
    this->_applyStyle(view, _StyleId::PKT_INDEX_BUILD_PROGRESS_VIEW_PATH,
                      filename ? _ATTR_BOLD : 0);
}

void Stylist::pktIndexBuildProgressViewBar(const View& view, const bool on) const
{
    if (on) {
        this->_applyStyle(view, _StyleId::STD, _ATTR_REVERSE | _ATTR_BOLD);
    } else {
        this->stdDim(view);
    }
}

void Stylist::error(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::TABLE_VIEW_ERROR_CELL, _ATTR_BOLD);
}

void Stylist::detailsViewTypeInfo(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_TYPE_INFO);
}

void Stylist::detailsViewDtName(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_DT_NAME, _ATTR_BOLD);
}

void Stylist::detailsViewDtExtra(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_DT_EXTRA, _ATTR_BOLD);
}

void Stylist::detailsViewMappingOrFlagName(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_MAPPING_OR_FLAG_NAME);
}

void Stylist::detailsViewMappingOrFlagRange(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_MAPPING_OR_FLAG_RANGE);
}

void Stylist::detailsViewPropKey(ncplane * const plane) const
{
    this->std(plane, true);
}

void Stylist::detailsViewPropVal(ncplane * const plane) const
{
    this->_applyStyle(plane, _StyleId::DETAILS_VIEW_PROP_VAL);
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
    this->_applyStyle(view, _StyleId::PKT_DECODING_ERROR_DETAILS_VIEW, emphasized ? _ATTR_BOLD : 0);
}

void Stylist::searchInputViewPrefix(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_PREFIX, _ATTR_BOLD);
}

void Stylist::searchInputViewAddSub(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ADD_SUB, _ATTR_BOLD);
}

void Stylist::searchInputViewWildcard(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_WILDCARD, _ATTR_BOLD);
}

void Stylist::searchInputViewEscape(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ESCAPE, _ATTR_BOLD);
}

void Stylist::searchInputViewNumber(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_NUMBER);
}

void Stylist::searchInputViewError(const View& view) const
{
    this->_applyStyle(view, _StyleId::SEARCH_INPUT_VIEW_ERROR, _ATTR_BOLD);
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

    this->_applyStyle(view, styleId, _ATTR_BOLD);
}

void Stylist::pktDataViewAuxSel(const View& view) const
{
    this->_applyStyle(view, _StyleId::STD, _ATTR_BOLD | _ATTR_REVERSE);
}

void Stylist::pktDataViewOffset(const View& view, const bool sel) const
{
    if (sel) {
        this->_applyStyle(view, _StyleId::PKT_DATA_VIEW_OFFSET_CUR, _ATTR_BOLD);
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
