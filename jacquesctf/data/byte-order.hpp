/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_BYTE_ORDER_HPP
#define _JACQUES_BYTE_ORDER_HPP

#include <boost/optional.hpp>

namespace jacques {

enum class ByteOrder
{
    BIG,
    LITTLE,
};

using OptByteOrder = boost::optional<ByteOrder>;

} // namespace jacques

#endif // _JACQUES_BYTE_ORDER_HPP
