/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_FROM_DT_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_FROM_DT_HPP

#include <vector>
#include <yactfr/yactfr.hpp>

#include "abstract-dt-details.hpp"
#include "../stylist.hpp"

namespace jacques {

void dtDetailsFromDt(const yactfr::DataType& dt, const Stylist& stylist,
                     std::vector<AbstractDtDetails::UP>& vec);

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_FROM_DT_HPP
