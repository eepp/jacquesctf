/*
 * SPDX-FileCopyrightText: 2026 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "color.hpp"

namespace jacques {
namespace {

/*
 * Color in the OKLab color space.
 *
 * See <https://bottosson.github.io/posts/oklab/>.
 */
struct Oklab final
{
    double l, a, b;
};

/*
 * Converts the sRGB component `comp` (0 to 255) to a linear
 * component (0 to 1).
 */
double linearCompFromSrgb(const std::uint8_t comp) noexcept
{
    const auto c = comp / 255.;

    return c <= .04045 ? c / 12.92 : std::pow((c + .055) / 1.055, 2.4);
}

/*
 * Converts the linear component `comp` (0 to 1) to an sRGB
 * component (0 to 255), clamping it if needed.
 */
std::uint8_t srgbCompFromLinear(const double comp) noexcept
{
    const auto c = comp <= .0031308 ? comp * 12.92 : 1.055 * std::pow(comp, 1 / 2.4) - .055;

    return static_cast<std::uint8_t>(std::clamp(c * 255. + .5, 0., 255.));
}

Oklab oklabFromColor(const Color& color) noexcept
{
    const auto r = linearCompFromSrgb(color.r());
    const auto g = linearCompFromSrgb(color.g());
    const auto b = linearCompFromSrgb(color.b());
    const auto l = std::cbrt(.4122214708 * r + .5363325363 * g + .0514459929 * b);
    const auto m = std::cbrt(.2119034982 * r + .6806995451 * g + .1073969566 * b);
    const auto s = std::cbrt(.0883024619 * r + .2817188376 * g + .6299787005 * b);

    return Oklab {
         .2104542553 * l +  .7936177850 * m - .0040720468 * s,
        1.9779984951 * l - 2.4285922050 * m + .4505937099 * s,
         .0259040371 * l +  .7827717662 * m - .8086757660 * s,
    };
}

Color colorFromOklab(const Oklab& oklab) noexcept
{
    const auto l = std::pow(oklab.l + .3963377774 * oklab.a +  .2158037573 * oklab.b, 3);
    const auto m = std::pow(oklab.l - .1055613458 * oklab.a -  .0638541728 * oklab.b, 3);
    const auto s = std::pow(oklab.l - .0894841775 * oklab.a - 1.2914855480 * oklab.b, 3);

    return Color {
        srgbCompFromLinear( 4.0767416621 * l - 3.3077115913 * m +  .2309699292 * s),
        srgbCompFromLinear(-1.2684380046 * l + 2.6097574011 * m -  .3413193965 * s),
        srgbCompFromLinear( -.0041960863 * l -  .7034186147 * m + 1.7076147010 * s),
    };
}

} // namespace

Color Color::mix(const Color& other, const double ratio) const noexcept
{
    const auto x = oklabFromColor(*this);
    const auto y = oklabFromColor(other);

    return colorFromOklab(Oklab {
        x.l + (y.l - x.l) * ratio,
        x.a + (y.a - x.a) * ratio,
        x.b + (y.b - x.b) * ratio,
    });
}

} // namespace jacques
