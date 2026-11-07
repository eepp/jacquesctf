/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <notcurses/notcurses.h>

#include "color.hpp"
#include "stylist.hpp"
#include "term.hpp"

namespace jacques {

namespace {

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
    /*
     * The fallback colors (0xRRGGBB) are for a terminal which doesn't
     * report its own (Tango basic colors for the palette).
     */
    const auto palColor = [](const unsigned int index, const std::uint32_t fallbackRgb) {
        return term::paletteColor(index).value_or(Color {fallbackRgb});
    };

    const auto fg = term::defaultFgColor().value_or(Color {0xd3e3e5});
    const auto bg = term::defaultBgColor().value_or(Color {0x081a1f});
    const auto red = palColor(1, 0xcc0000);
    const auto green = palColor(2, 0x4e9a06);
    const auto yellow = palColor(3, 0xc4a000);
    const auto blue = palColor(4, 0x3465a4);
    const auto magenta = palColor(5, 0x75507b);
    const auto cyan = palColor(6, 0x06989a);
    const auto brightRed = palColor(9, 0xef2929);
    const auto brightYellow = palColor(11, 0xfce94f);
    const auto brightBlue = palColor(12, 0x729fcf);
    const auto brightMagenta = palColor(13, 0xad7fa8);
    const auto brightCyan = palColor(14, 0x34e2e2);

    // colors mixed from the ones above
    const auto selBg = cyan.mix(bg, .3);
    const auto darkBlueBg = blue.mix(bg, .5);
    const auto infoBg = magenta.mix(bg, .4);
    const auto errorBg = red.mix(bg, .4);

    /*
     * The styles which have the default background color as their
     * foreground color (`INVERSE` and some bookmarks) aren't bold.
     *
     * A terminal which doesn't support RGB colors gets palette colors
     * instead (notcurses quantizes them), and many terminals show a
     * bold foreground with the bright version of its palette color:
     * black would become gray and hard to read.
     */
    const struct
    {
        Role role;
        Color fg;
        Color bg;
        unsigned int ncStyles;
    } table[] = {
        {Role::STD,          fg,                     bg,                0},
        {Role::DIM,          fg.mix(bg, .4),         bg,                0},
        {Role::FAINT,        fg.mix(bg, .65),        bg,                0},
        {Role::INVERSE,      bg,                     fg,                0},
        {Role::TITLE,        brightCyan,             bg,                NCSTYLE_BOLD | NCSTYLE_UNDERLINE},
        {Role::SUBTITLE,     green,                  bg,                NCSTYLE_UNDERLINE},
        {Role::HIGHLIGHT,    yellow,                 bg,                0},
        {Role::ACCENT,       cyan,                   bg,                0},
        {Role::SOFT_ACCENT,  fg.mix(cyan, .5),       bg,                0},
        {Role::UNIT,         fg.mix(blue, .5), bg,                0},
        {Role::SYMBOL,       brightMagenta,          bg,                0},
        {Role::PROP_KEY,     fg,                     bg,                NCSTYLE_BOLD},
        {Role::PROP_VAL,     green,                  bg,                0},
        {Role::PATH,         brightBlue,             bg,                0},
        {Role::NUMBER,       brightBlue,             bg,                0},
        {Role::WARNING,      brightYellow,           bg,                NCSTYLE_BOLD},
        {Role::ERROR,        brightRed,              bg,                NCSTYLE_BOLD},
        {Role::ALERT,        fg,                     errorBg,           0},
        {Role::SEL,          fg,                     selBg,             NCSTYLE_BOLD},
        {Role::SEL_WARNING,  brightYellow,           selBg,             NCSTYLE_BOLD},
        {Role::SEL_ERROR,    fg,                     errorBg,           NCSTYLE_BOLD},
        {Role::SEL_PREV,     brightMagenta,          bg,                NCSTYLE_BOLD},
        {Role::SEL_NEXT,     green,                  bg,                NCSTYLE_BOLD},
        {Role::OFFSET,       yellow.mix(bg, .15),    bg,                0},
        {Role::CUR_OFFSET,   brightYellow,           bg,                NCSTYLE_BOLD},
        {Role::PADDING,      blue,                   bg,                0},
        {Role::BOOKMARK_1,   bg,                     yellow,            0},
        {Role::BOOKMARK_2,   bg,                     green,             0},
        {Role::BOOKMARK_3,   fg,                     blue,              0},
        {Role::BOOKMARK_4,   fg,                     magenta,           0},
        {Role::STATUS,       fg,                     darkBlueBg,        0},
        {Role::INFO,         fg.mix(infoBg, .3),     infoBg,            0},
        {Role::INFO_STRONG,  fg,                     infoBg,            NCSTYLE_BOLD},
        {Role::INFO_VAL,     brightYellow,           infoBg,            NCSTYLE_BOLD},
        {Role::INFO_ERROR,   red,                    fg,                NCSTYLE_BOLD},
        {Role::INPUT_BORDER, bg,                     green,             0},
        {Role::DT_NAME,      fg,                     darkBlueBg,        NCSTYLE_BOLD},
        {Role::DT_EXTRA,     fg,                     green.mix(bg, .5), NCSTYLE_BOLD},
    };

    _styles.resize(std::size(table));

    for (auto& entry : table) {
        const auto index = static_cast<std::size_t>(entry.role);

        assert(index < _styles.size());

        auto& style = _styles[index];

        assert(style.channels == 0);
        style.channels = ncChannels(entry.fg, entry.bg);
        style.ncStyles = entry.ncStyles;
    }
}

void Stylist::apply(ncplane& plane, const Role role, const bool isEm) const
{
    const auto index = static_cast<std::size_t>(role);

    assert(index < _styles.size());

    const auto& style = _styles[index];

    ncplane_set_channels(&plane, style.channels);
    ncplane_set_styles(&plane, style.ncStyles | (isEm ? NCSTYLE_BOLD : 0));
}

} // namespace jacques
