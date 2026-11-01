/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_KEY_HPP
#define _JACQUES_INSPECT_COMMAND_UI_KEY_HPP

#include <cwctype>
#include <optional>
#include <variant>

namespace jacques {

/*
 * Special (non-character) key.
 */
enum class SpecialKey
{
    // not a real key: the terminal was resized
    RESIZE,

    UP,
    DOWN,
    LEFT,
    RIGHT,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    BACKSPACE,
    ENTER,
    TAB,
    ESCAPE,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
};

/*
 * Character of Ctrl+`letter`, where `letter` is an uppercase ASCII
 * letter (for example, U+000C for Ctrl+L).
 */
constexpr char32_t ctrlCh(const char letter) noexcept
{
    return static_cast<char32_t>(letter - 'A' + 1);
}

/*
 * A user key (see term::readKey()): either a character (Unicode code
 * point, including the control characters of Ctrl+A to Ctrl+Z; see
 * ctrlCh()) or a special key.
 */
class Key final
{
public:
    explicit constexpr Key(const char32_t ch) noexcept :
        _val {ch}
    {
    }

    explicit constexpr Key(const SpecialKey specialKey) noexcept :
        _val {specialKey}
    {
    }

    /*
     * Character of this key, if it's not a special key.
     */
    constexpr std::optional<char32_t> ch() const noexcept
    {
        if (const auto ch = std::get_if<char32_t>(&_val)) {
            return *ch;
        }

        return std::nullopt;
    }

    /*
     * Special key of this key, if it's not a character.
     */
    constexpr std::optional<SpecialKey> special() const noexcept
    {
        if (const auto specialKey = std::get_if<SpecialKey>(&_val)) {
            return *specialKey;
        }

        return std::nullopt;
    }

    /*
     * Character of this key if it's printable according to the current
     * locale (see std::iswprint()).
     */
    std::optional<char32_t> printableCh() const noexcept
    {
        if (const auto ch = this->ch(); ch && std::iswprint(static_cast<std::wint_t>(*ch))) {
            return *ch;
        }

        return std::nullopt;
    }

private:
    std::variant<char32_t, SpecialKey> _val;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_KEY_HPP
