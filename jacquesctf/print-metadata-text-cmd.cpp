/*
 * SPDX-FileCopyrightText: 2019 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <fmt/format.h>

#include "cfg.hpp"
#include "data/trace.hpp"
#include "data/ds-file.hpp"
#include "print-metadata-text-cmd.hpp"

namespace jacques {

void printMetadataTextCmd(const PrintMetadataTextCfg& cfg)
{
    // not appending any newline to print the exact text
    fmt::print("{}", Trace {cfg.path(), {}}.metadata().text());
}

} // namespace jacques
