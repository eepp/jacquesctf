/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_BO_HPP
#define _JACQUES_DATA_BO_HPP

#include <boost/optional.hpp>

namespace jacques {

enum class Bo
{
    BIG,
    LITTLE,
};

using OptBo = boost::optional<Bo>;

} // namespace jacques

#endif // _JACQUES_DATA_BO_HPP
