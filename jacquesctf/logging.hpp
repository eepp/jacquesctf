/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_LOGGING_HPP
#define _JACQUES_LOGGING_HPP

#include "spdlog/spdlog.h"

namespace jacques {

extern std::shared_ptr<spdlog::logger> theLogger;

} // namespace jacques

#endif // _JACQUES_LOGGING_HPP
