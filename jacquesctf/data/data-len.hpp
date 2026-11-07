/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_DATA_LEN_HPP
#define _JACQUES_DATA_DATA_LEN_HPP

#include <string>
#include <string_view>
#include <optional>
#include <variant>
#include <boost/operators.hpp>
#include <fmt/format.h>

#include "aliases.hpp"
#include "utils.hpp"

namespace jacques {

/*
 * Data length formatted with `utils::LenFmtMode::FULL_FLOOR` or
 * `utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS`.
 */
struct FullFmtDataLen final
{
    // quantity of `unit` with three decimal places
    std::string qty;

    // `B`, `KiB`, `MiB`, or `GiB`
    std::string unit;

    /*
     * Extra bits (only with
     * `utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS`).
     */
    std::optional<std::string> extraBits;
};

/*
 * Data length formatted with `utils::LenFmtMode::BYTES_FLOOR` or
 * `utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS`.
 */
struct BytesFmtDataLen final
{
    std::string bytes;

    /*
     * Extra bits (only
     * with `utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS`).
     */
    std::optional<std::string> extraBits;
};

// data length formatted with `utils::LenFmtMode::BITS`
struct BitsFmtDataLen final
{
    std::string bits;
};

using FmtDataLen = std::variant<FullFmtDataLen, BytesFmtDataLen, BitsFmtDataLen>;

class DataLen final :
    public boost::totally_ordered<DataLen>
{
public:
    [[ nodiscard ]] static DataLen fromBytes(const Size lenBytes) noexcept
    {
        return {lenBytes * 8};
    }

public:
    explicit DataLen() noexcept = default;
    DataLen(const DataLen&) noexcept = default;

    // non-explicit makes this easier to use
    DataLen(const Size lenBits) noexcept :
        _lenBits {lenBits}
    {
    }

    DataLen& operator=(const DataLen&) noexcept = default;

    /*
     * Formats this data length with the format mode `fmtMode`, using
     * `sep` as the thousands separator, if set, for the
     * `utils::LenFmtMode::BYTES_FLOOR*` and
     * `utils::LenFmtMode::BITS` modes.
     */
    FmtDataLen format(utils::LenFmtMode fmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                      const std::optional<char>& sep = std::nullopt) const;

    bool hasExtraBits() const noexcept
    {
        return this->extraBits() > 0;
    }

    Size extraBits() const noexcept
    {
        return _lenBits & 7;
    }

    Size bits() const noexcept
    {
        return _lenBits;
    }

    Size operator*() const noexcept
    {
        return _lenBits;
    }

    Size bytes() const noexcept
    {
        return _lenBits / 8;
    }

    bool operator==(const DataLen& other) const noexcept
    {
        return _lenBits == other._lenBits;
    }

    bool operator<(const DataLen& other) const noexcept
    {
        return _lenBits < other._lenBits;
    }

    DataLen operator+(const DataLen& len) const noexcept
    {
        return _lenBits + len._lenBits;
    }

    DataLen operator-(const DataLen& len) const noexcept
    {
        return _lenBits - len._lenBits;
    }

    DataLen& operator+=(const DataLen& len) noexcept
    {
        _lenBits += len._lenBits;
        return *this;
    }

    DataLen& operator+=(const Size lenBits) noexcept
    {
        _lenBits += lenBits;
        return *this;
    }

    DataLen& operator-=(const DataLen& len) noexcept
    {
        _lenBits -= len._lenBits;
        return *this;
    }

    DataLen& operator-=(const Size lenBits) noexcept
    {
        _lenBits -= lenBits;
        return *this;
    }

private:
    Size _lenBits = 0;
};

static inline DataLen operator+(const Size left, const DataLen& right) noexcept
{
    return left + right.bits();
}

static inline DataLen operator-(const Size left, const DataLen& right) noexcept
{
    return left - right.bits();
}

static inline bool operator<(const Size left, const DataLen& right) noexcept
{
    return left < right.bits();
}

static inline bool operator<=(const Size left, const DataLen& right) noexcept
{
    return left <= right.bits();
}

static inline bool operator==(const Size left, const DataLen& right) noexcept
{
    return left == right.bits();
}

static inline bool operator!=(const Size left, const DataLen& right) noexcept
{
    return left != right.bits();
}

static inline bool operator>(const Size left, const DataLen& right) noexcept
{
    return left > right.bits();
}

static inline bool operator>=(const Size left, const DataLen& right) noexcept
{
    return left >= right.bits();
}

static inline DataLen operator""_KiB(const Size kib) noexcept
{
    return DataLen::fromBytes(kib * 1024);
}

static inline DataLen operator""_MiB(const Size mib) noexcept
{
    return DataLen::fromBytes(mib * 1024 * 1024);
}

static inline DataLen operator""_GiB(const Size gib) noexcept
{
    return DataLen::fromBytes(gib * 1024 * 1024 * 1024);
}

} // namespace jacques

template <>
struct fmt::formatter<jacques::DataLen> : fmt::formatter<std::string_view>
{
    auto format(const jacques::DataLen& dataLen, fmt::format_context& ctx) const
    {
        const auto fmtLen = std::get<jacques::FullFmtDataLen>(
            dataLen.format(jacques::utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS));
        auto str = fmtLen.qty;

        if (fmtLen.extraBits) {
            str += '+' + *fmtLen.extraBits;
        }

        str += ' ' + fmtLen.unit;
        return fmt::formatter<std::string_view>::format(str, ctx);
    }
};

#endif // _JACQUES_DATA_DATA_LEN_HPP
