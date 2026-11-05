/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <notcurses/notcurses.h>

#include "term.hpp"
#include "key.hpp"
#include "cmd-error.hpp"
#include "unicode.hpp"

namespace jacques {
namespace term {
namespace {

/*
 * Deleter of a notcurses context: restores the terminal.
 */
struct NcDeleter final
{
    void operator()(notcurses * const nc) const noexcept
    {
        notcurses_stop(nc);
    }
};

std::unique_ptr<notcurses, NcDeleter> theNc;
bool theCursorIsVisible = false;

/*
 * Converts the notcurses input `ni` (having the ID `id`) to a user key,
 * or returns `std::nullopt` to ignore it.
 */
std::optional<Key> keyFromInput(const std::uint32_t id, const ncinput& ni) noexcept
{
    if (ni.evtype == NCTYPE_RELEASE) {
        return std::nullopt;
    }

    if (ni.modifiers & (NCKEY_MOD_ALT | NCKEY_MOD_SUPER | NCKEY_MOD_HYPER | NCKEY_MOD_META)) {
        return std::nullopt;
    }

    const auto specialKey = std::invoke([id]() -> std::optional<SpecialKey> {
        switch (id) {
        case NCKEY_RESIZE:
            return SpecialKey::RESIZE;

        case NCKEY_UP:
            return SpecialKey::UP;

        case NCKEY_DOWN:
            return SpecialKey::DOWN;

        case NCKEY_LEFT:
            return SpecialKey::LEFT;

        case NCKEY_RIGHT:
            return SpecialKey::RIGHT;

        case NCKEY_PGUP:
            return SpecialKey::PAGE_UP;

        case NCKEY_PGDOWN:
            return SpecialKey::PAGE_DOWN;

        case NCKEY_HOME:
            return SpecialKey::HOME;

        case NCKEY_END:
            return SpecialKey::END;

        case NCKEY_BACKSPACE:
            return SpecialKey::BACKSPACE;

        case NCKEY_ENTER:
            return SpecialKey::ENTER;

        case NCKEY_TAB:
            return SpecialKey::TAB;

        case NCKEY_ESC:
            return SpecialKey::ESCAPE;

        default:
            break;
        }

        if (id >= NCKEY_F01 && id <= NCKEY_F12) {
            return static_cast<SpecialKey>(static_cast<int>(SpecialKey::F1) +
                                           static_cast<int>(id - NCKEY_F01));
        }

        return std::nullopt;
    });

    if (specialKey) {
        return Key {*specialKey};
    }

    if (nckey_synthesized_p(id)) {
        // other special key (including lone modifier keys): ignore
        return std::nullopt;
    }

    if (ni.modifiers & NCKEY_MOD_CTRL) {
        // notcurses reports Ctrl+A to Ctrl+Z as `A` to `Z` with Ctrl
        if (id >= 'A' && id <= 'Z') {
            return Key {ctrlCh(static_cast<char>(id))};
        }

        return std::nullopt;
    }

    /*
     * Prefer the effective text, which takes the modifiers into account
     * (for example, Shift+3 → `#` with the kitty keyboard protocol).
     */
    return Key {ni.eff_text[0] != 0 ? ni.eff_text[0] : id};
}

} // namespace

void init()
{
    assert(!theNc);

    {
        notcurses_options opts {};

        opts.flags = NCOPTION_SUPPRESS_BANNERS;
        theNc.reset(notcurses_core_init(&opts, nullptr));
    }

    if (!theNc) {
        throw CmdError {"Cannot initialize the terminal."};
    }

    // disable cursor block
    theCursorIsVisible = false;

    /*
     * Jacques CTF only uses RGB colors, but notcurses quantizes them
     * when the terminal doesn't support them: require either RGB
     * colors or at least the basic palette colors.
     */
    if (!notcurses_cantruecolor(theNc.get()) && notcurses_palette_size(theNc.get()) < 8) {
        fini();
        throw CmdError {"Cannot continue: terminal must support at least 8 colors."};
    }

    if (!sizeOk()) {
        fini();
        throw CmdError {"Cannot continue: terminal size must be at least 80×16."};
    }
}

void fini() noexcept
{
    theNc.reset();
}

notcurses& nc() noexcept
{
    assert(theNc);
    return *theNc;
}

ncplane& stdPlane() noexcept
{
    return *notcurses_stdplane(&nc());
}

Rect size() noexcept
{
    unsigned int rows, cols;

    notcurses_term_dim_yx(&nc(), &rows, &cols);
    return Rect {{0, 0}, cols, rows};
}

bool sizeOk() noexcept
{
    const auto termSize = size();

    return termSize.w >= 80 && termSize.h >= 16;
}

namespace {

/*
 * Returns whether or not `str` only contains printable ASCII
 * characters, in which case its width is its length.
 */
bool isPrintableAscii(const std::string_view str) noexcept
{
    return std::all_of(str.begin(), str.end(), [](const char ch) {
        return ch >= 0x20 && ch < 0x7f;
    });
}

/*
 * Returns the offsets of the code point boundaries of the UTF-8 string
 * `str`, including 0 and `str.size()`.
 */
std::vector<Size> cpBoundaries(const std::string_view str)
{
    std::vector<Size> boundaries {0};
    auto rest = str;

    while (!rest.empty()) {
        unicode::popFrontUtf8(rest);
        boundaries.push_back(str.size() - rest.size());
    }

    return boundaries;
}

} // namespace

Size strWidth(const std::string_view str)
{
    if (isPrintableAscii(str)) {
        return str.size();
    }

    // ncstrwidth() needs a null-terminated string
    const std::string nullTermStr {str};

    // `validWidth` is the width of the whole string if it's valid
    auto validWidth = 0;

    ncstrwidth(nullTermStr.c_str(), nullptr, &validWidth);
    return static_cast<Size>(std::max(validWidth, 0));
}

std::string_view strFittingPrefix(const std::string_view str, const Size maxWidth)
{
    if (isPrintableAscii(str)) {
        return str.substr(0, maxWidth);
    }

    if (strWidth(str) <= maxWidth) {
        return str;
    }

    /*
     * The width of a prefix never decreases as the prefix grows: find
     * the longest one which fits with a binary search on the code
     * point boundaries.
     *
     * Invariant: the prefix ending at `boundaries[lo]` fits, while the
     * one ending at `boundaries[hi]` doesn't.
     */
    const auto boundaries = cpBoundaries(str);
    Index lo = 0;
    Index hi = boundaries.size() - 1;

    while (hi - lo > 1) {
        const auto mid = lo + (hi - lo) / 2;

        if (strWidth(str.substr(0, boundaries[mid])) <= maxWidth) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return str.substr(0, boundaries[lo]);
}

std::string_view strFittingSuffix(const std::string_view str, const Size maxWidth)
{
    if (isPrintableAscii(str)) {
        return str.substr(str.size() - std::min(static_cast<Size>(str.size()), maxWidth));
    }

    if (strWidth(str) <= maxWidth) {
        return str;
    }

    /*
     * Like in strFittingPrefix(), but the width of a suffix never
     * increases as it shrinks.
     *
     * Invariant: the suffix starting at `boundaries[lo]` doesn't fit,
     * while the one starting at `boundaries[hi]` does.
     */
    const auto boundaries = cpBoundaries(str);
    Index lo = 0;
    Index hi = boundaries.size() - 1;

    while (hi - lo > 1) {
        const auto mid = lo + (hi - lo) / 2;

        if (strWidth(str.substr(boundaries[mid])) <= maxWidth) {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    // skip zero-width code points which belong to the preceding character
    while (hi < boundaries.size() - 1 &&
           strWidth(str.substr(boundaries[hi], boundaries[hi + 1] - boundaries[hi])) == 0) {
        ++hi;
    }

    return str.substr(boundaries[hi]);
}

void update()
{
    notcurses_render(&nc());
}

void repaint()
{
    // this also resizes the standard plane to the new terminal size
    notcurses_refresh(&nc(), nullptr, nullptr);
}

Key readKey()
{
    while (true) {
        ncinput ni;
        const auto id = notcurses_get_blocking(&nc(), &ni);

        if (id == static_cast<std::uint32_t>(-1)) {
            continue;
        }

        if (const auto key = keyFromInput(id, ni)) {
            return *key;
        }
    }
}

void showCursor(const Point& pos)
{
    notcurses_cursor_enable(&nc(), static_cast<int>(pos.y), static_cast<int>(pos.x));
    theCursorIsVisible = true;
}

void hideCursor()
{
    if (!theCursorIsVisible) {
        return;
    }

    notcurses_cursor_disable(&nc());
    theCursorIsVisible = false;
}

bool cursorIsVisible() noexcept
{
    return theCursorIsVisible;
}

std::optional<Color> defaultFgColor() noexcept
{
    std::uint32_t rgb;

    if (notcurses_default_foreground(&nc(), &rgb) != 0) {
        return std::nullopt;
    }

    return Color {rgb};
}

std::optional<Color> defaultBgColor() noexcept
{
    std::uint32_t rgb;

    if (notcurses_default_background(&nc(), &rgb) != 0) {
        return std::nullopt;
    }

    return Color {rgb};
}

std::optional<Color> paletteColor(const unsigned int index) noexcept
{
    const std::unique_ptr<ncpalette, decltype(&ncpalette_free)> pal {ncpalette_new(&nc()),
                                                                     ncpalette_free};

    if (!pal || index >= NCPALETTESIZE) {
        return std::nullopt;
    }

    /*
     * notcurses doesn't say whether or not the terminal reported its
     * palette: if it didn't, then all the entries are zero.
     */
    if (std::all_of(std::begin(pal->chans), std::end(pal->chans),
                    [](const auto chan) {
                        return ncchannel_rgb(chan) == 0;
                    })) {
        return std::nullopt;
    }

    return Color {ncchannel_rgb(pal->chans[index])};
}

} // namespace term
} // namespace jacques
