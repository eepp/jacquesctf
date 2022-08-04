/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>

#include "cfg.hpp"
#include "data/trace.hpp"
#include "data/ds-file.hpp"
#include "print-metadata-text-cmd.hpp"

namespace jacques {

void printMetadataTextCmd(const PrintMetadataTextCfg& cfg)
{
    // not appending any newline to print the exact text
    std::cout << Trace {cfg.path(), {}}.metadata().text();
    std::cout.flush();
}

} // namespace jacques
