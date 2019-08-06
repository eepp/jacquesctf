/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>

#include "config.hpp"
#include "metadata.hpp"
#include "print-metadata-text-command.hpp"

namespace jacques {

void printMetadataTextCommand(const PrintMetadataTextConfig& cfg)
{
    const Metadata metadata {cfg.path()};

    // not appending any newline to print the exact text
    std::cout << metadata.text();
    std::cout.flush();
}

} // namespace jacques
