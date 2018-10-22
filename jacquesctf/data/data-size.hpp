/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_SIZE_HPP
#define _JACQUES_DATA_SIZE_HPP

#include <utility>
#include <tuple>
#include <string>
#include <ostream>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"
#include "utils.hpp"

namespace jacques {

class DataSize :
    public boost::totally_ordered<DataSize>
{
public:
    static DataSize fromBytes(Size sizeBytes)
    {
        return {sizeBytes * 8};
    }

public:
    DataSize() = default;
    DataSize(const DataSize&) = default;

    // non explicit makes this easier to use
    DataSize(const Size sizeBits) :
        _sizeBits {sizeBits}
    {
    }

    DataSize& operator=(const DataSize&) = default;
    std::pair<std::string, std::string> format(utils::SizeFormatMode formatMode = utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                               const boost::optional<char>& sep = boost::none) const;

    bool hasExtraBits() const noexcept
    {
        return this->extraBits() > 0;
    }

    Size extraBits() const noexcept
    {
        return _sizeBits & 7;
    }

    Size bits() const noexcept
    {
        return _sizeBits;
    }

    Size bytes() const noexcept
    {
        return _sizeBits / 8;
    }

    bool operator==(const DataSize& other) const noexcept
    {
        return _sizeBits == other._sizeBits;
    }

    bool operator<(const DataSize& other) const noexcept
    {
        return _sizeBits < other._sizeBits;
    }

    DataSize operator+(const DataSize& size) const noexcept
    {
        return _sizeBits + size._sizeBits;
    }

    DataSize operator+(const Size size) const noexcept
    {
        return _sizeBits + size;
    }

    DataSize operator-(const DataSize& size) const noexcept
    {
        return _sizeBits - size._sizeBits;
    }

    DataSize operator-(const Size size) const noexcept
    {
        return _sizeBits - size;
    }

    DataSize& operator+=(const DataSize& size) noexcept
    {
        _sizeBits += size._sizeBits;
        return *this;
    }

    DataSize& operator+=(const Size size) noexcept
    {
        _sizeBits += size;
        return *this;
    }

    DataSize& operator-=(const DataSize& size) noexcept
    {
        _sizeBits -= size._sizeBits;
        return *this;
    }

    DataSize& operator-=(const Size size) noexcept
    {
        _sizeBits -= size;
        return *this;
    }

private:
    Size _sizeBits = 0;
};

static inline
std::ostream& operator<<(std::ostream& stream, const DataSize& dataSize)
{
    std::string qty, units;

    std::tie(qty, units) = dataSize.format(utils::SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                           ',');
    stream << qty << ' ' << units;
    return stream;
}

static inline
DataSize operator""_kiB(const Size kib) noexcept
{
    return DataSize::fromBytes(kib * 1024);
}

static inline
DataSize operator""_MiB(const Size mib) noexcept
{
    return DataSize::fromBytes(mib * 1024 * 1024);
}

static inline
DataSize operator""_GiB(const Size gib) noexcept
{
    return DataSize::fromBytes(gib * 1024 * 1024 * 1024);
}

} // namespace jacques

#endif // _JACQUES_DATA_SIZE_HPP
