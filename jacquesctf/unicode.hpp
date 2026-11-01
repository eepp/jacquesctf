/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_UNICODE_HPP
#define _JACQUES_UNICODE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <yactfr/yactfr.hpp>

namespace jacques::unicode {

/*
 * Replacement character (U+FFFD), which stands for an invalid or
 * unrepresentable character.
 */
constexpr char32_t replacementCh = U'�';

/*
 * Returns true if `cp` is a Unicode scalar value, that is, a code point
 * which isn't a surrogate.
 */
constexpr bool isScalarVal(const char32_t cp) noexcept
{
    return cp <= 0x10ffff && (cp < 0xd800 || cp > 0xdfff);
}

/*
 * Appends the UTF-8 encoding of `cp` to `str`, or the one of
 * `replacementCh` if `cp` isn't a scalar value.
 */
void appendUtf8(std::string& str, char32_t cp);

/*
 * Removes the last code point of the UTF-8 string `str`, if any.
 */
void popBackUtf8(std::string& str) noexcept;

/*
 * Removes the first code point of the UTF-8 string `str` (not empty)
 * and returns it.
 *
 * If `str` starts with an invalid or truncated UTF-8 sequence, then
 * this function removes the longest invalid prefix (at least one byte)
 * and returns `replacementCh`.
 */
char32_t popFrontUtf8(std::string_view& str) noexcept;

/*
 * Decodes the bytes from `begin` to `end`, encoded with `encoding`, up
 * to the first encoded U+0000 (excluded), if any, and returns the
 * equivalent UTF-8 string.
 *
 * The returned string is always valid UTF-8: this function replaces
 * each invalid or truncated sequence with `replacementCh`.
 */
std::string toUtf8(const std::uint8_t *begin, const std::uint8_t *end,
                   yactfr::StringEncoding encoding);

} // namespace jacques::unicode

#endif // _JACQUES_UNICODE_HPP
