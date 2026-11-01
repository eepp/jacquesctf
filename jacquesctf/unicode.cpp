/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <yactfr/yactfr.hpp>

#include "unicode.hpp"

namespace jacques::unicode {

void appendUtf8(std::string& str, char32_t cp)
{
    if (!isScalarVal(cp)) {
        cp = replacementCh;
    }

    if (cp < 0x80) {
        str += static_cast<char>(cp);
    } else if (cp < 0x800) {
        str += static_cast<char>(0xc0 | (cp >> 6));
        str += static_cast<char>(0x80 | (cp & 0x3f));
    } else if (cp < 0x10000) {
        str += static_cast<char>(0xe0 | (cp >> 12));
        str += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        str += static_cast<char>(0x80 | (cp & 0x3f));
    } else {
        str += static_cast<char>(0xf0 | (cp >> 18));
        str += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
        str += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        str += static_cast<char>(0x80 | (cp & 0x3f));
    }
}

void popBackUtf8(std::string& str) noexcept
{
    // remove the continuation bytes (0b10xxxxxx), then the leading byte
    while (!str.empty() && (static_cast<std::uint8_t>(str.back()) & 0xc0) == 0x80) {
        str.pop_back();
    }

    if (!str.empty()) {
        str.pop_back();
    }
}

char32_t popFrontUtf8(std::string_view& str) noexcept
{
    assert(!str.empty());

    const auto byteAt = [&str](const std::size_t index) {
        return static_cast<std::uint8_t>(str[index]);
    };

    const auto leadingByte = byteAt(0);

    if (leadingByte < 0x80) {
        str.remove_prefix(1);
        return leadingByte;
    }

    /*
     * Sequence length, initial code point bits, and valid range of the
     * first continuation byte (see the "Well-Formed UTF-8 Byte
     * Sequences" table of the Unicode standard, which excludes overlong
     * encodings, surrogates, and code points above U+10FFFF).
     */
    std::size_t len;
    char32_t cp;
    std::uint8_t minContByte = 0x80;
    std::uint8_t maxContByte = 0xbf;

    if (leadingByte >= 0xc2 && leadingByte <= 0xdf) {
        len = 2;
        cp = leadingByte & 0x1f;
    } else if (leadingByte >= 0xe0 && leadingByte <= 0xef) {
        len = 3;
        cp = leadingByte & 0x0f;

        if (leadingByte == 0xe0) {
            minContByte = 0xa0;
        } else if (leadingByte == 0xed) {
            maxContByte = 0x9f;
        }
    } else if (leadingByte >= 0xf0 && leadingByte <= 0xf4) {
        len = 4;
        cp = leadingByte & 0x07;

        if (leadingByte == 0xf0) {
            minContByte = 0x90;
        } else if (leadingByte == 0xf4) {
            maxContByte = 0x8f;
        }
    } else {
        // continuation byte or invalid leading byte
        str.remove_prefix(1);
        return replacementCh;
    }

    for (std::size_t i = 1; i < len; ++i) {
        if (i >= str.size() || byteAt(i) < minContByte || byteAt(i) > maxContByte) {
            // truncated or invalid: remove the valid prefix
            str.remove_prefix(i);
            return replacementCh;
        }

        cp = (cp << 6) | (byteAt(i) & 0x3f);

        // only the first continuation byte has a special range
        minContByte = 0x80;
        maxContByte = 0xbf;
    }

    str.remove_prefix(len);
    return cp;
}

namespace {

/*
 * Reads a code unit of `size` bytes at `p` with the byte
 * order `isBigEndian`.
 */
std::uint32_t readCodeUnit(const std::uint8_t *p, const unsigned int size,
                           const bool isBigEndian) noexcept
{
    std::uint32_t unit = 0;

    for (auto i = 0U; i < size; ++i) {
        const auto byte = static_cast<std::uint32_t>(p[i]);

        if (isBigEndian) {
            unit = (unit << 8) | byte;
        } else {
            unit |= byte << (8 * i);
        }
    }

    return unit;
}

void appendUtf8FromUtf8(std::string& str, const std::uint8_t * const begin,
                        const std::uint8_t * const end)
{
    std::string_view rest {
        reinterpret_cast<const char *>(begin), static_cast<std::size_t>(end - begin)
    };

    while (!rest.empty()) {
        const auto cp = popFrontUtf8(rest);

        if (cp == 0) {
            return;
        }

        appendUtf8(str, cp);
    }
}

void appendUtf8FromUtf16(std::string& str, const std::uint8_t * const begin,
                         const std::uint8_t * const end, const bool bigEndian)
{
    auto it = begin;

    while (end - it >= 2) {
        const auto unit = readCodeUnit(it, 2, bigEndian);

        it += 2;

        if (unit == 0) {
            return;
        }

        if (unit >= 0xd800 && unit <= 0xdbff) {
            // high surrogate: must be followed with a low surrogate
            if (end - it >= 2) {
                const auto lowUnit = readCodeUnit(it, 2, bigEndian);

                if (lowUnit >= 0xdc00 && lowUnit <= 0xdfff) {
                    it += 2;
                    appendUtf8(str, 0x10000 + ((unit - 0xd800) << 10) + (lowUnit - 0xdc00));
                    continue;
                }
            }

            appendUtf8(str, replacementCh);
            continue;
        }

        // appendUtf8() replaces a lone low surrogate
        appendUtf8(str, unit);
    }

    if (it != end) {
        // truncated code unit
        appendUtf8(str, replacementCh);
    }
}

void appendUtf8FromUtf32(std::string& str, const std::uint8_t * const begin,
                         const std::uint8_t * const end, const bool bigEndian)
{
    auto it = begin;

    while (end - it >= 4) {
        const auto unit = readCodeUnit(it, 4, bigEndian);

        it += 4;

        if (unit == 0) {
            return;
        }

        // appendUtf8() replaces a non-scalar value
        appendUtf8(str, unit);
    }

    if (it != end) {
        // truncated code unit
        appendUtf8(str, replacementCh);
    }
}

} // namespace

std::string toUtf8(const std::uint8_t * const begin, const std::uint8_t * const end,
                   const yactfr::StringEncoding encoding)
{
    assert(begin <= end);

    std::string str;

    switch (encoding) {
    case yactfr::StringEncoding::Utf8:
        appendUtf8FromUtf8(str, begin, end);
        break;

    case yactfr::StringEncoding::Utf16Be:
    case yactfr::StringEncoding::Utf16Le:
        appendUtf8FromUtf16(str, begin, end, encoding == yactfr::StringEncoding::Utf16Be);
        break;

    case yactfr::StringEncoding::Utf32Be:
    case yactfr::StringEncoding::Utf32Le:
        appendUtf8FromUtf32(str, begin, end, encoding == yactfr::StringEncoding::Utf32Be);
        break;
    }

    return str;
}

} // namespace jacques::unicode
