/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_DATA_LEN_HPP
#define _JACQUES_DATA_DATA_LEN_HPP

#include <utility>
#include <tuple>
#include <string>
#include <ostream>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"
#include "utils.hpp"

namespace jacques {

class DataLen final :
    public boost::totally_ordered<DataLen>
{
public:
    static DataLen fromBytes(const Size lenBytes) noexcept
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

    std::pair<std::string, std::string> format(utils::LenFmtMode fmtMode = utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                               const boost::optional<char>& sep = boost::none) const;

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

static inline std::ostream& operator<<(std::ostream& stream, const DataLen& dataLen)
{
    std::string qty, units;

    std::tie(qty, units) = dataLen.format(utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS, ',');
    stream << qty << ' ' << units;
    return stream;
}

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

#endif // _JACQUES_DATA_DATA_LEN_HPP
