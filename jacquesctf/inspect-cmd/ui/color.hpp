/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_COLOR_HPP
#define _JACQUES_INSPECT_COMMAND_UI_COLOR_HPP

#include <cstdint>

namespace jacques {

/*
 * 24-bit RGB color.
 */
class Color final
{
public:
    explicit constexpr Color(const std::uint8_t r, const std::uint8_t g,
                             const std::uint8_t b) noexcept :
        _r {r},
        _g {g},
        _b {b}
    {
    }

    /*
     * Builds a color from the notcurses 24-bit RGB value `rgb`
     * (0xRRGGBB), ignoring its 8 most significant bits.
     */
    explicit constexpr Color(const std::uint32_t rgb) noexcept :
        _r {static_cast<std::uint8_t>((rgb >> 16) & 0xff)},
        _g {static_cast<std::uint8_t>((rgb >> 8) & 0xff)},
        _b {static_cast<std::uint8_t>(rgb & 0xff)}
    {
    }

    /*
     * Red component.
     */
    constexpr std::uint8_t r() const noexcept
    {
        return _r;
    }

    /*
     * Green component.
     */
    constexpr std::uint8_t g() const noexcept
    {
        return _g;
    }

    /*
     * Blue component.
     */
    constexpr std::uint8_t b() const noexcept
    {
        return _b;
    }

    /*
     * Equivalent notcurses 24-bit RGB value (0xRRGGBB) of this color.
     */
    constexpr std::uint32_t ncRgb() const noexcept
    {
        return (static_cast<std::uint32_t>(_r) << 16) | (static_cast<std::uint32_t>(_g) << 8) |
               static_cast<std::uint32_t>(_b);
    }

    /*
     * Returns a new color which is `ratio` of the way from this color
     * to `other`, `ratio` being between 0 (this color) and 1 (`other`).
     *
     * This method interpolates in the OKLab color space so that the
     * result looks evenly in between to the human eye.
     */
    Color mix(const Color& other, double ratio) const noexcept;

    constexpr bool operator==(const Color& other) const noexcept
    {
        return _r == other._r && _g == other._g && _b == other._b;
    }

    constexpr bool operator!=(const Color& other) const noexcept
    {
        return !(*this == other);
    }

private:
    std::uint8_t _r, _g, _b;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_COLOR_HPP
