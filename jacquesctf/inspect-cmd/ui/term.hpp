/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_TERM_HPP
#define _JACQUES_INSPECT_COMMAND_UI_TERM_HPP

#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "aliases.hpp"
#include "color.hpp"
#include "point.hpp"
#include "rect.hpp"
#include "key.hpp"

struct notcurses;
struct ncplane;

namespace jacques {
namespace term {

/*
 * Takes TOTAL CONTROL of the terminal.
 *
 * Call this before any other function of this namespace, except
 * mutex(), and call fini() when you're done to give the terminal back
 * to the user.
 *
 * The terminal must not be initialized (call fini() first to initialize
 * it again).
 *
 * Throws `CmdError` if the terminal supports neither RGB colors nor at
 * least 8 palette colors, or if it's too small (see sizeOk()). In that
 * case, the terminal remains uninitialized.
 */
void init();

/*
 * Restores the terminal to its state before init().
 *
 * Call this before printing anything to the standard output or
 * standard error, including when an exception escapes, as the
 * terminal isn't usable as is until then.
 *
 * Does nothing if the terminal isn't initialized.
 */
void fini() noexcept;

/*
 * notcurses context.
 */
notcurses& nc() noexcept;

/*
 * Standard plane (always at the bottom, covering the whole terminal).
 *
 * Lock mutex() while modifying it if another thread could render at
 * the same time.
 */
ncplane& stdPlane() noexcept;

/*
 * Current terminal size, as a rectangle at (0, 0).
 *
 * Only repaint() updates this size.
 */
Rect size() noexcept;

/*
 * Returns true if the terminal has the minimum dimensions.
 */
bool sizeOk() noexcept;

/*
 * Number of terminal columns which the UTF-8 string `str` occupies as
 * notcurses computes it when it prints `str`.
 *
 * If `str` contains an invalid sequence or a control character, then
 * this function returns the width of the part before it.
 */
Size strWidth(std::string_view str);

/*
 * Returns the longest prefix of the UTF-8 string `str` which occupies
 * at most `maxWidth` terminal columns (see strWidth()).
 *
 * The returned prefix never ends in the middle of a character, and
 * includes the zero-width characters (combining marks, for example)
 * which follow its last character.
 */
std::string_view strFittingPrefix(std::string_view str, Size maxWidth);

/*
 * Returns the longest suffix of the UTF-8 string `str` which occupies
 * at most `maxWidth` terminal columns (see strWidth()).
 *
 * The returned suffix never starts in the middle of a character, nor
 * with a zero-width character (combining mark, for example) which
 * belongs to the preceding character.
 */
std::string_view strFittingSuffix(std::string_view str, Size maxWidth);

/*
 * Renders all the visible planes to the terminal.
 *
 * Locks mutex() itself.
 */
void update();

/*
 * Clears and repaints the whole terminal with the last rendered
 * content (for example, if something else corrupted it).
 *
 * This also updates the terminal dimensions: call it after a
 * `SpecialKey::RESIZE` key.
 *
 * Locks mutex() itself.
 */
void repaint();

/*
 * Blocks until the user presses a key and returns it.
 *
 * Doesn't lock mutex() so that other threads may keep rendering while
 * this function waits.
 */
Key readKey();

/*
 * Shows the terminal cursor at the absolute position `pos`.
 *
 * This writes to the terminal immediately: lock mutex() if another
 * thread could render at the same time.
 */
void showCursor(const Point& pos);

/*
 * Hides the terminal cursor.
 *
 * This writes to the terminal immediately: lock mutex() if another
 * thread could render at the same time.
 */
void hideCursor();

/*
 * Returns true if the terminal cursor is visible, that is, if the last
 * call to showCursor() happened after the last call to hideCursor().
 *
 * The cursor is hidden after init().
 */
bool cursorIsVisible() noexcept;

/*
 * Default foreground color of the terminal, if known.
 */
std::optional<Color> defaultFgColor() noexcept;

/*
 * Default background color of the terminal, if known.
 */
std::optional<Color> defaultBgColor() noexcept;

/*
 * Color of the entry at the index `index` of the palette of the
 * terminal, if known.
 *
 * The palette is unknown when the terminal doesn't report it (notcurses
 * queries it in init()), in which case all its entries are black.
 */
std::optional<Color> paletteColor(unsigned int index) noexcept;

/*
 * Mutex to lock while modifying planes or writing to the terminal when
 * more than one thread could do so (see InspectScreen::_search()).
 *
 * update() and repaint() lock it themselves.
 */
std::recursive_mutex& mutex() noexcept;

} // namespace term
} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_TERM_HPP
