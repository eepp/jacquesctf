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

#include "cfg.hpp"
#include "copy-pkts-cmd.hpp"
#include "cmd-error.hpp"
#include "io-error.hpp"
#include "data/trace.hpp"
#include "data/ds-file.hpp"
#include "data/time-ops.hpp"

namespace jacques {

static Index pktIndexSpecToIndex(const std::string& pktIndexSpec, const Size pktCount)
{
    std::string absIndexSpec = pktIndexSpec;
    auto fromLast = false;

    if (pktIndexSpec[0] == ':') {
        fromLast = true;
        absIndexSpec = pktIndexSpec.substr(1);
    }

    const auto pktIndexSpecNumber = std::stoull(absIndexSpec);

    if (pktIndexSpecNumber == 0) {
        throw CmdError {"0 is not a valid packet index"};
    }

    if (!fromLast) {
        const auto pktIndex = pktIndexSpecNumber - 1;

        if (pktIndex >= pktCount) {
            std::ostringstream ss;

            ss << pktIndexSpecNumber << " is out of range (" << pktCount << " packets)";
            throw CmdError {ss.str()};
        }

        return pktIndex;
    }

    if (pktIndexSpecNumber > pktCount) {
            std::ostringstream ss;

            ss << pktIndexSpecNumber << " is out of range (" << pktCount << " packets)";
            throw CmdError {ss.str()};
    }

    return pktCount - pktIndexSpecNumber;
}

static std::vector<Index> parsePktIndexSpecList(const std::string& pktIndexSpecList,
                                                const Size pktCount)
{
    const std::regex indexRe {"^\\s*(:?\\d+)\\s*"};
    const std::regex rangeRe {"^\\s*(:?\\d+)\\s*\\.\\.\\s*(:?\\d+)\\s*"};
    std::vector<Index> indexes;
    auto charIt = pktIndexSpecList.begin();
    const auto endCharIt = pktIndexSpecList.end();

    while (charIt < endCharIt) {
        std::smatch matchRes;

        // try range first because it starts with an index
        if (std::regex_search(charIt, endCharIt, matchRes, rangeRe)) {
            const auto first = pktIndexSpecToIndex(matchRes[1], pktCount);
            const auto second = pktIndexSpecToIndex(matchRes[2], pktCount);

            if (first <= second) {
                for (auto i = first; i <= second; ++i) {
                    indexes.push_back(i);
                }
            } else {
                for (auto i = static_cast<long long>(first);
                        i >= static_cast<long long>(second); --i) {
                    indexes.push_back(static_cast<Index>(i));
                }
            }
        } else if (std::regex_search(charIt, endCharIt, matchRes, indexRe)) {
            indexes.push_back(pktIndexSpecToIndex(matchRes[1], pktCount));
        } else {
            std::ostringstream ss;

            ss << "unknown token found: `" << std::string {charIt, endCharIt} << "`";
            throw CmdError {ss.str()};
        }

        charIt += matchRes.length(0);
    }

    if (indexes.empty()) {
        throw CmdError {"no indexes specified"};
    }

    return indexes;
}

static void copyPkt(std::ifstream& srcStream, const PktIndexEntry& indexEntry,
                    std::ofstream& dstStream, std::vector<char>& buf)
{
    const auto offset = indexEntry.offsetInDsFileBytes();
    const auto size = indexEntry.effectiveTotalLen().bytes();
    auto remSize = size;

    srcStream.seekg(offset);

    while (remSize > 0) {
        const auto readSize = std::min(static_cast<Size>(buf.size()), remSize);

        srcStream.read(buf.data(), readSize);
        remSize -= readSize;
        dstStream.write(buf.data(), readSize);
    }
}

static std::vector<Index> tryParsePktIndexSpecList(const std::string& pktIndexSpecList,
                                                   const Size pktCount)
{
    try {
        return parsePktIndexSpecList(pktIndexSpecList, pktCount);
    } catch (const CmdError& exc) {
        std::ostringstream ss;

        ss << "Invalid packet indexes: `" << pktIndexSpecList << "`: " << exc.what() << ".";
        throw CmdError {ss.str()};
    }
}

void copyPktsCmd(const CopyPktsCfg& cfg)
{
    Trace trace {{cfg.srcPath()}};
    auto& dsf = *trace.dsFiles().front();

    dsf.buildIndex();

    if (dsf.pktCount() == 0) {
        throw CmdError {"File is empty."};
    }

    const auto indexes = tryParsePktIndexSpecList(cfg.pktIndexes(), dsf.pktCount());
    std::ifstream srcStream {cfg.srcPath().c_str(), std::ios::binary};
    std::ofstream dstStream;

    srcStream.exceptions(std::ios::badbit | std::ios::failbit);
    dstStream.exceptions(std::ios::badbit | std::ios::failbit);

    try {
        dstStream.open(cfg.dstPath().c_str(), std::ios::binary);
        std::vector<char> buf(64 * 1024);

        for (const auto index : indexes) {
            copyPkt(srcStream, dsf.pktIndexEntry(index), dstStream, buf);
        }

        srcStream.close();
        dstStream.close();
    } catch (const std::ios_base::failure& exc) {
        throw CmdError {exc.what()};
    }
}

} // namespace jacques
