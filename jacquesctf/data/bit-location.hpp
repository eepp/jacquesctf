/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_BIT_LOCATION_HPP
#define _JACQUES_DATA_BIT_LOCATION_HPP

#include <cstdint>

#include "aliases.hpp"
#include "data/data-size.hpp"

namespace jacques {

/*
 * A bit location indicates where a requested bit is within a byte
 * buffer. Its two properties are:
 *
 * Byte index:
 *     The index of the first byte containing the requested bit from the
 *     beginning of the byte buffer.
 *
 * Bit index in byte:
 *     The index of the requested bit within its byte, where 0 is the
 *     LSB's index and 7 is the MSB's index.
 *
 * For example, given this byte buffer:
 *
 *     10100110 11110110 00101101 00101001
 *                ^  *
 *
 * Both bits are in byte 1. The `^` bit is at index 5 within its byte
 * while the `*` bit is at index 2. Both bits could be at the same
 * offset in the buffer depending on their byte order: `^` is at offset
 * 10 in big endian and `*` is at offset 10 in little endian.
 */
class BitLocation
{
public:
    explicit BitLocation(const Index byteIndex,
                         const Index bitIndexInByte) :
        _byteIndex {byteIndex},
        _bitIndexInByte {bitIndexInByte}
    {
    }

    BitLocation(const BitLocation&) = default;
    BitLocation& operator=(const BitLocation&) = default;

    Index byteIndex() const noexcept
    {
        return _byteIndex;
    }

    Index bitIndexInByte() const noexcept
    {
        return _bitIndexInByte;
    }

private:
    Index _byteIndex;
    Index _bitIndexInByte;
};

} // namespace jacques

#endif // _JACQUES_DATA_BIT_LOCATION_HPP
