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
#include "data-len.hpp"
#include "bit-loc.hpp"
#include "bo.hpp"

namespace jacques {

/*
 * A bit array contains actual data from the packet. Its lifetime
 * depends on the lifetime of the packet which created it
 * (see Pkt::bitArray()).
 *
 * You can use bitLoc() to get the bit location of a bit at a given
 * index within the bit array. You can use operator[]() to get the value
 * of a specific bit from its location or index.
 */
class BitArray final
{
public:
    explicit BitArray(const std::uint8_t * const buf, const Index offsetInFirstByteBits,
                      const DataLen& len, const OptBo& bo = boost::none) noexcept :
        _buf {buf},
        _offsetInFirstByteBits {offsetInFirstByteBits},
        _len {len},
        _bo {bo}
    {
    }

    BitArray(const BitArray&) noexcept = default;
    BitArray& operator=(const BitArray&) noexcept = default;

    /*
     * The offset, within the first byte, of the beginning of this bit
     * array.
     */
    Index offsetInFirstByteBits() const noexcept
    {
        return _offsetInFirstByteBits;
    }

    const DataLen& len() const noexcept
    {
        return _len;
    }

    // byte buffer of this bit array
    const std::uint8_t *buf() const noexcept
    {
        return _buf;
    }

    const OptBo& bo() const noexcept
    {
        return _bo;
    }

    BitLoc bitLoc(Index index) const noexcept
    {
        index += this->offsetInFirstByteBits();

        const auto byteIndex = index / 8;
        Index bitIndexInByte;

        if (!_bo || _bo == yactfr::ByteOrder::BIG) {
            bitIndexInByte = 7 - (index - byteIndex * 8);
        } else {
            bitIndexInByte = index - byteIndex * 8;
        }

        return BitLoc {byteIndex, bitIndexInByte};
    }

    std::uint8_t operator[](const BitLoc& bitLoc) const noexcept
    {
        return (_buf[bitLoc.byteIndex()] >> bitLoc.bitIndexInByte()) & 1;
    }

    std::uint8_t operator[](const Index index) const noexcept
    {
        return (*this)[this->bitLoc(index)];
    }

private:
    const std::uint8_t *_buf = nullptr;
    Index _offsetInFirstByteBits = 0;
    DataLen _len = 0;
    OptBo _bo;
};

} // namespace jacques

#endif // _JACQUES_DATA_BIT_ARRAY_HPP
