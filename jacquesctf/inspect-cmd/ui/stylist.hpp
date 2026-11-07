/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP
#define _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP

#include <cstdint>
#include <vector>

struct ncplane;

namespace jacques {

/*
 * A stylist holds the style (foreground color, background color, and
 * attributes) of each role (see `Role`) and applies it to a plane.
 *
 * Its constructor builds the colors from the colors of the terminal.
 */
class Stylist final
{
public:
    /*
     * What some text is, which determines its style.
     */
    enum class Role
    {
        // regular text and focused view border
        STD,

        // less important text (value not available, nonprintable byte)
        DIM,

        // blurred view border and table column separator
        FAINT,

        // inverted regular text (table header, progress bar)
        INVERSE,

        // section title
        TITLE,

        // subsection title
        SUBTITLE,

        /*
         * Something to notice: emphasized view border, "more content"
         * indicator, emphasized table column, mapping or flag name.
         */
        HIGHLIGHT,

        /*
         * Secondary part of a value or expression: mapping or flag
         * range, search prefix.
         */
        ACCENT,

        /*
         * Subtler `ACCENT` (nanosecond part of a table timestamp
         * or duration).
         */
        SOFT_ACCENT,

        // unit of a table value (`MiB`, `ns`, `cc`)
        UNIT,

        /*
         * Symbol: key in the help, data type kind, search wildcard,
         * "more text" arrow.
         */
        SYMBOL,

        // property name
        PROP_KEY,

        // property value
        PROP_VAL,

        // file system path
        PATH,

        // number in a search expression
        NUMBER,

        // warning
        WARNING,

        // error
        ERROR,

        // text on an error background (error details, invalid input)
        ALERT,

        // selected item
        SEL,

        // selected item with a warning
        SEL_WARNING,

        // selected item with an error
        SEL_ERROR,

        // previous and next packet regions of the selected one
        SEL_PREV,
        SEL_NEXT,

        // offsets of the packet data view, and current one
        OFFSET,
        CUR_OFFSET,

        // padding packet region
        PADDING,

        // packet region bookmarks
        BOOKMARK_1,
        BOOKMARK_2,
        BOOKMARK_3,
        BOOKMARK_4,

        // status bar
        STATUS,

        // packet region info bar: regular text, important text, value, error
        INFO,
        INFO_STRONG,
        INFO_VAL,
        INFO_ERROR,

        // input box border
        INPUT_BORDER,

        // name and extra information of a data type
        DT_NAME,
        DT_EXTRA,
    };

public:
    Stylist();

    Stylist(const Stylist&) = delete;
    Stylist& operator=(const Stylist&) = delete;

    /*
     * Makes the next writes to `plane` use the style of `role`, bold
     * if `isEm` is true.
     */
    void apply(ncplane& plane, Role role, bool isEm = false) const;

private:
    struct _Style final
    {
        // notcurses channels (foreground/background colors)
        std::uint64_t channels = 0;

        // notcurses styles (`NCSTYLE_*`)
        unsigned int ncStyles = 0;
    };

private:
    // indexed by role
    std::vector<_Style> _styles;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_STYLIST_HPP
