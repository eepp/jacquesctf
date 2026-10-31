/*
 * SPDX-FileCopyrightText: 2019 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
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
