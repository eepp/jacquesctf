/*
 * Copyright (C) 2019 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cassert>
#include <regex>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>

#include "config.hpp"
#include "copy-packets-command.hpp"
#include "command-error.hpp"
#include "io-error.hpp"
#include "metadata.hpp"
#include "data-stream-file.hpp"
#include "time-ops.hpp"

namespace jacques {

static Index packetIndexSpecToIndex(const std::string& packetIndexSpec,
                                    const Size packetCount)
{
    std::string absIndexSpec = packetIndexSpec;
    auto fromLast = false;

    if (packetIndexSpec[0] == ':') {
        fromLast = true;
        absIndexSpec = packetIndexSpec.substr(1);
    }

    const auto packetIndexSpecNumber = std::stoull(absIndexSpec);

    if (packetIndexSpecNumber == 0) {
        throw CommandError {"`0` is not a valid packet index"};
    }

    Index packetIndex;

    if (!fromLast) {
        packetIndex = packetIndexSpecNumber - 1;

        if (packetIndex >= packetCount) {
            std::ostringstream ss;

            ss << packetIndexSpecNumber << " is out of range (" <<
                  packetCount << " packets)";
            throw CommandError {ss.str()};
        }

        return packetIndex;
    }

    if (packetIndexSpecNumber > packetCount) {
            std::ostringstream ss;

            ss << packetIndexSpecNumber << " is out of range (" <<
                  packetCount << " packets)";
            throw CommandError {ss.str()};
    }

    return packetCount - packetIndexSpecNumber;
}

static std::vector<Index> parsePacketIndexSpecList(const std::string& packetIndexSpecList,
                                                   const Size packetCount)
{
    static const std::regex indexRe {"^\\s*(:?\\d+)\\s*"};
    static const std::regex rangeRe {"^\\s*(:?\\d+)\\s*\\.\\.\\s*(:?\\d+)\\s*"};
    std::vector<Index> indexes;
    auto charIt = std::begin(packetIndexSpecList);
    const auto endCharIt = std::end(packetIndexSpecList);

    while (charIt < endCharIt) {
        std::smatch matchRes;

        // try range first because it starts with an index
        if (std::regex_search(charIt, endCharIt, matchRes, rangeRe)) {
            const auto first = packetIndexSpecToIndex(matchRes[1],
                                                      packetCount);
            const auto second = packetIndexSpecToIndex(matchRes[2],
                                                       packetCount);

            if (first <= second) {
                for (auto i = first; i <= second; ++i) {
                    indexes.push_back(i);
                }
            } else {
                for (long long i = static_cast<long long>(first);
                        i >= static_cast<long long>(second); --i) {
                    indexes.push_back(static_cast<Index>(i));
                }
            }
        } else if (std::regex_search(charIt, endCharIt, matchRes, indexRe)) {
            indexes.push_back(packetIndexSpecToIndex(matchRes[1], packetCount));
        } else {
            std::ostringstream ss;

            ss << "unknown token found: `" << std::string {charIt, endCharIt} << "`";
            throw CommandError {ss.str()};
        }

        charIt += matchRes.length(0);
    }

    if (indexes.empty()) {
        throw CommandError {"no indexes specified"};
    }

    return indexes;
}

void copyPacket(std::ifstream& srcStream, const PacketIndexEntry& indexEntry,
                std::ofstream& dstStream, std::vector<char>& buf)
{
    const auto offset = indexEntry.offsetInDataStreamFileBytes();
    const auto size = indexEntry.effectiveTotalSize().bytes();
    auto remSize = size;

    srcStream.seekg(offset);

    while (remSize > 0) {
        const auto readSize = std::min(static_cast<Size>(buf.size()), remSize);

        srcStream.read(buf.data(), readSize);
        remSize -= readSize;
        dstStream.write(buf.data(), readSize);
    }
}

void copyPacketsCommand(const CopyPacketsConfig& cfg)
{
    const Metadata metadata {cfg.srcPath().parent_path() / "metadata"};
    DataStreamFile dsf {cfg.srcPath(), metadata};

    dsf.buildIndex();

    if (dsf.packetCount() == 0) {
        throw CommandError {"File is empty."};
    }

    std::vector<Index> indexes;

    try {
        indexes = parsePacketIndexSpecList(cfg.packetIndexes(),
                                           dsf.packetCount());
    } catch (const CommandError& ex) {
        std::ostringstream ss;

        ss << "Invalid packet indexes: `" << cfg.packetIndexes() << "`: " <<
              ex.what() << ".";
        throw CommandError {ss.str()};
    }

    std::ifstream srcStream {cfg.srcPath().c_str(), std::ios::binary};
    std::ofstream dstStream;

    srcStream.exceptions(std::ios::badbit | std::ios::failbit);
    dstStream.exceptions(std::ios::badbit | std::ios::failbit);

    try {
        dstStream.open(cfg.dstPath().c_str(), std::ios::binary);
        std::vector<char> buf(64 * 1024);

        for (const auto index : indexes) {
            copyPacket(srcStream, dsf.packetIndexEntry(index), dstStream, buf);
        }

        srcStream.close();
        dstStream.close();
    } catch (const std::ios_base::failure& ex) {
        throw CommandError {ex.what()};
    }
}

} // namespace jacques
