/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_BIT_ARRAY_HPP
#define _JACQUES_DATA_BIT_ARRAY_HPP

#include <cstdint>
#include <boost/optional.hpp>

#include "aliases.hpp"
#include "data/data-size.hpp"
#include "bit-location.hpp"
#include "byte-order.hpp"

namespace jacques {

/*
 * A bit array contains actual data from the packet. Its lifetime
 * depends on the lifetime of the packet which created it
 * (see Packet::bitArray()).
 *
 * You can use bitLocation() to get the bit location of a bit at a given
 * index within the bit array. You can use operator[]() to get the value
 * of a specific bit from its location or index.
 */
class BitArray
{
public:
    explicit BitArray(const std::uint8_t *buf,
                      const Index offsetInFirstByteBits, const DataSize& size,
                      const OptByteOrder& byteOrder = boost::none) :
        _buf {buf},
        _offsetInFirstByteBits {offsetInFirstByteBits},
        _size {size},
        _byteOrder {byteOrder}
    {
    }

    BitArray(const BitArray&) = default;
    BitArray& operator=(const BitArray&) = default;

    // the offset, within the first byte, of this bit array's beginning
    Index offsetInFirstByteBits() const noexcept
    {
        return _offsetInFirstByteBits;
    }

    const DataSize& size() const noexcept
    {
        return _size;
    }

    // this bit array's byte buffer
    const std::uint8_t *buf() const noexcept
    {
        return _buf;
    }

    const OptByteOrder& byteOrder() const noexcept
    {
        return _byteOrder;
    }

    BitLocation bitLocation(Index index) const noexcept
    {
        index += this->offsetInFirstByteBits();

        const auto byteIndex = index / 8;
        Index bitIndexInByte;

        if (!_byteOrder || _byteOrder == ByteOrder::BIG) {
            bitIndexInByte = 7 - (index - byteIndex * 8);
        } else {
            bitIndexInByte = index - byteIndex * 8;
        }

        return BitLocation {byteIndex, bitIndexInByte};
    }

    std::uint8_t operator[](const BitLocation& bitLoc) const noexcept
    {
        return (_buf[bitLoc.byteIndex()] >> bitLoc.bitIndexInByte()) & 1;
    }

    std::uint8_t operator[](const Index index) const noexcept
    {
        return (*this)[this->bitLocation(index)];
    }

private:
    const std::uint8_t *_buf = nullptr;
    Index _offsetInFirstByteBits = 0;
    DataSize _size = 0;
    OptByteOrder _byteOrder;
};

} // namespace jacques

#endif // _JACQUES_DATA_BIT_ARRAY_HPP
