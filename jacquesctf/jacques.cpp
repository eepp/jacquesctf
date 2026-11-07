/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cstdlib>
#include <memory>
#include <fmt/format.h>

#include "cfg.hpp"
#include "utils.hpp"
#include "print-metadata-text-cmd.hpp"
#include "list-pkts-cmd.hpp"
#include "copy-pkts-cmd.hpp"
#include "create-lttng-index-cmd.hpp"

#ifdef JACQUES_HAS_INSPECT_CMD
# include "inspect-cmd/ui/inspect-cmd.hpp"
#endif

namespace jacques {
namespace {

void printCliUsage(const char * const cmdName)
{
    fmt::print(R"(Usage: {} [GEN OPTS] [CMD] ARG...

General options:

  --help, -h     Print usage and exit
  --version, -V  Print version and exit

`inspect` command (default)
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
)"
#ifdef JACQUES_HAS_INSPECT_CMD
R"(Usage: inspect PATH...

Interactively inspect CTF traces, CTF data stream files, or CTF metadata
stream files.

If PATH is a single CTF metadata file, print its text content and exit.
If PATH is a CTF data stream file, inspect this file.
If PATH is a directory, inspect all CTF data stream files found recursively.
)"
#else
R"(Not available in this build.
)"
#endif
R"(
`list-packets` command
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Usage: list-packets --machine [--header] [--with-er-count] PATH

Print the list of packets of CTF data stream file PATH and their properties.

Options:

  --header         Print table header
  --machine, -m    Print machine-readable data (CSV)
  --with-er-count  Analyze each packet to print its event record count
                   (slow; empty for an invalid packet)

`copy-packets` command
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Usage: copy-packets SRC-PATH PACKET-INDEXES DST-PATH

Copy the specific packets PACKET-INDEXES from the CTF data stream file SRC-PATH
to the new CTF data stream file DST-PATH.

PACKET-INDEXES is a single argument containing the indexes of the packets to
copy from SRC-PATH:

* The first packet is considered to have index 1.

* The packet indexes are space-delimited.

* You can copy a range of packets with the `A..B` format (packet A to B,
  inclusively). A can be greater than B to reverse the copy order.

* You can copy the Nth last packet with the `:N` format, where `:1` means the
  last packet.

Examples:

    17
    1 2 3 9 10
    2 3 7..11 16..21 :4 :1
    2 9 :10..:5
    1 :5..4 99
    1..:1
    2..:1
    1..:2

`create-lttng-index` command
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Usage: create-lttng-index PATH...

Create an LTTng index file for each specified CTF data stream file.

If PATH is a CTF data stream file, use this file.
If PATH is a directory, use all the CTF data stream files found recursively.
)", cmdName);
}

void printVersion()
{
    fmt::print("Jacques CTF {}\n", JACQUES_VERSION);
}

void jacques(const int argc, const char *argv[])
{
    const auto cfg = cfgFromArgs(argc, argv);

    if (dynamic_cast<const PrintCliUsageCfg *>(cfg.get())) {
        printCliUsage(argv[0]);
    } else if (dynamic_cast<const PrintVersionCfg *>(cfg.get())) {
        printVersion();
    } else if (const auto specCfg = dynamic_cast<const PrintMetadataTextCfg *>(cfg.get())) {
        printMetadataTextCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const ListPktsCfg *>(cfg.get())) {
        listPktsCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CopyPktsCfg *>(cfg.get())) {
        copyPktsCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const CreateLttngIndexCfg *>(cfg.get())) {
        createLttngIndexCmd(*specCfg);
    } else if (const auto specCfg = dynamic_cast<const InspectCfg *>(cfg.get())) {
#ifdef JACQUES_HAS_INSPECT_CMD
        inspectCmd(*specCfg);
#else
        throw CliError {"`inspect` command requested, but not available in this build."};
#endif
    } else {
        std::abort();
    }
}

} // namespace
} // namespace jacques

int main(const int argc, const char *argv[])
{
    const auto exStr = jacques::utils::tryFunc([argc, argv] {
        jacques::jacques(argc, argv);
    });

    if (exStr) {
        jacques::utils::printError(*exStr);
        return 1;
    }
}
