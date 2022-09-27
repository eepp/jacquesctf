/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cstring>
#include <cstdio>
#include <array>
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/algorithm/string/join.hpp>
#include <algorithm>

#include "utils.hpp"
#include "cfg.hpp"

namespace jacques {
namespace utils {

namespace bfs = boost::filesystem;

std::pair<std::string, std::string> formatPath(const bfs::path& path, const Size maxLen)
{
    const auto filename = path.filename().string();
    const auto dirName = path.parent_path().string();
    std::string filenameStr;

    if (filename.size() > maxLen) {
        filenameStr = "...";
        filenameStr.append(filename.c_str() + filename.size() - maxLen + 3);
    } else {
        filenameStr = filename;
    }

    auto leftLen = maxLen - filenameStr.size();
    std::string dirNameStr;

    if (leftLen >= 4) {
        // enough space for `...` and implicit `/`

        if (dirName.size() < leftLen) {
            dirNameStr = dirName;
        } else {
            dirNameStr = "...";
            dirNameStr.append(dirName.c_str() + dirName.size() - leftLen + 4);
        }
    }

    return std::make_pair(std::move(dirNameStr), std::move(filenameStr));
}

namespace {

inline long long abs(const long long val) noexcept
{
    return std::abs(val);
}

inline unsigned long long abs(const unsigned long long val) noexcept
{
    return val;
}

template <typename ValT>
std::string sepNumber(const ValT val, const char sep, const char * const fmt)
{
    /*
     * 1. Convert absolute value to string.
     *
     * 2. Create return string, iterate first string in reverse order,
     *    and append separator every three characters.
     *
     * 3. Remove trailing separator if any.
     *
     * 4. Append `-` if the value is negative.
     *
     * 5. Reverse the whole string.
     */
    std::array<char, 64> buf;
    const auto count = std::sprintf(buf.data(), fmt, abs(val));
    Index i = 0;
    std::string ret;

    ret.reserve(count + count / 3 + 1);

    for (auto at = count - 1; at >= 0; --at, ++i) {
        const auto ch = buf[at];

        if (i % 3 == 0 && i != 0) {
            ret += sep;
        }

        ret += ch;
    }

    if (ret.back() == sep) {
        ret.pop_back();
    }

    if (val < 0) {
        ret += '-';
    }

    std::reverse(ret.begin(), ret.end());
    return ret;
}

} // namespace

std::string sepNumber(const long long val, const char sep)
{
    return sepNumber(val, sep, "%lld");
}

std::string sepNumber(const unsigned long long val, const char sep)
{
    return sepNumber(val, sep, "%llu");
}

std::string wrapText(const std::string& text, const Size lineLen)
{
    return boost::algorithm::join(wrapTextLines(text, lineLen), "\n");
}

std::vector<std::string> wrapTextLines(const std::string& text, const Size lineLen)
{
    if (text.empty()) {
        return {};
    }

    std::istringstream words {text};
    std::vector<std::string> lines;
    std::string word;

    lines.push_back({});

    auto curLine = &lines.back();

    curLine->append(word);

    auto spaceLeft = lineLen - curLine->size();

    while (words >> word) {
        if (spaceLeft < word.size() + 1) {
            lines.push_back(word);
            curLine = &lines.back();
            spaceLeft = lineLen - word.size();
        } else {
            if (!curLine->empty()) {
                curLine->push_back(' ');
            }

            curLine->append(word);
            spaceLeft -= word.size() + 1;
        }
    }

    return lines;
}

std::string normalizeGlobPattern(const std::string& pattern)
{
    std::string normPat;
    auto gotStar = false;

    normPat.reserve(pattern.size());

    for (const auto patCh : pattern) {
        switch (patCh) {
        case '*':
            if (gotStar) {
                // avoid consecutive stars
                continue;
            }

            gotStar = true;
            break;

        default:
            gotStar = false;
            break;
        }

        /* Copy single character. */
        normPat += patCh;
    }

    return normPat;
}

namespace {

inline bool atEndOfPattern(const char * const p, const std::string& pattern) noexcept
{
    return static_cast<Size>(p - pattern.c_str()) == pattern.size() || *p == '\0';
}

} // namespace

bool globMatch(const std::string& pattern, const std::string& candidate)
{
    auto retryC = candidate.c_str();
    auto retryP = pattern.c_str();
    const char *c, *p;
    auto gotAStar = false;
    const auto candidateLen = candidate.size();

retry:
    c = retryC;
    p = retryP;

    while (static_cast<Size>(c - candidate.c_str()) < candidateLen && *c != '\0') {
        assert(*c);

        if (atEndOfPattern(p, pattern)) {
            goto endOfPattern;
        }

        switch (*p) {
        case '*':
            gotAStar = true;

            /*
             * Our first try starts at the current candidate
             * character and after the star in the pattern.
             */
            retryC = c;
            retryP = p + 1;

            if (atEndOfPattern(retryP, pattern)) {
                /*
                 * Star at the end of the pattern at this point:
                 * automatic match.
                 */
                return true;
            }

            goto retry;

        case '\\':
            /* Go to escaped character. */
            ++p;

            /*
             * Fall through the default case which will
             * compare the escaped character now.
             */

            // fall through!

        default:
            if (atEndOfPattern(p, pattern) || *c != *p) {
endOfPattern:
                /* Character mismatch OR end of pattern. */
                if (!gotAStar) {
                    /*
                     * We didn't get any star yet, so this first
                     * mismatch automatically makes the whole test fail.
                     */
                    return false;
                }

                /*
                 * Next try: next candidate character, original pattern
                 * character (following the most recent star).
                 */
                ++retryC;
                goto retry;
            }
            break;
        }

        /* Next pattern and candidate characters. */
        ++c;
        ++p;
    }

    /*
     * We checked every candidate character and we're still in a success
     * state: the only pattern character allowed to remain is a star.
     */
    if (atEndOfPattern(p, pattern)) {
        return true;
    }

    ++p;
    return p[-1] == '*' && atEndOfPattern(p, pattern);
}

std::pair<std::string, std::string> formatLen(const Size lenBits, const LenFmtMode fmtMode,
                                              const boost::optional<char>& sep)
{
    std::array<char, 64> buf;
    const char *unit = nullptr;
    const auto sizeBytes = lenBits / 8;
    const auto extraBits = lenBits & 7;

    switch (fmtMode) {
    case LenFmtMode::FULL_FLOOR:
    case LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS:
    {
        unit = "B";
        double val = static_cast<double>(sizeBytes);

        if (sizeBytes >= 1024 * 1024 * 1024) {
            val = static_cast<double>(sizeBytes) / (1024. * 1024 * 1024);
            unit = "GiB";
        } else if (sizeBytes >= 1024 * 1024) {
            val = static_cast<double>(sizeBytes) / (1024. * 1024);
            unit = "MiB";
        } else if (sizeBytes >= 1024) {
            val = static_cast<double>(sizeBytes) / 1024.;
            unit = "KiB";
        }

        if (fmtMode == LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS && extraBits > 0) {
            std::sprintf(buf.data(), "%.3f+%llu", val, extraBits);
        } else {
            std::sprintf(buf.data(), "%.3f", val);
        }

        break;
    }

    case LenFmtMode::BYTES_FLOOR:
    case LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS:
        unit = "B";

        if (fmtMode == LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS && extraBits > 0) {
            if (sep) {
                const auto sepNum = sepNumber(static_cast<long long>(sizeBytes), *sep);

                std::sprintf(buf.data(), "%s+%llu", sepNum.c_str(), extraBits);
            } else {
                std::sprintf(buf.data(), "%llu+%llu", sizeBytes, extraBits);
            }
        } else {
            if (sep) {
                const auto sepNum = sepNumber(static_cast<long long>(sizeBytes), *sep);

                std::strcpy(buf.data(), sepNum.c_str());
            } else {
                std::sprintf(buf.data(), "%llu", sizeBytes);
            }
        }

        break;

    case LenFmtMode::BITS:
        unit = "b";

        if (sep) {
            const auto sepNum = sepNumber(static_cast<long long>(lenBits), *sep);

            std::strcpy(buf.data(), sepNum.c_str());
        } else {
            std::sprintf(buf.data(), "%llu", lenBits);
        }

        break;
    }

    assert(unit);
    return {buf.data(), unit};
}

std::pair<std::string, std::string> formatNs(long long ns, const boost::optional<char>& sep)
{
    constexpr auto nsInS = 1'000'000'000LL;
    const auto absNs = std::abs(ns);
    const auto absSOnly = (absNs / nsInS);
    const auto nsOnly = absNs % nsInS;
    std::string sStr;

    if (ns < 0) {
        sStr = "-";
    }

    sStr += sep ? sepNumber(absSOnly, *sep) : std::to_string(absSOnly);

    const auto nsStr = sep ? sepNumber(nsOnly, *sep, "%09llu") :
                       std::to_string(nsOnly);

    return {sStr, nsStr};
}

void printTextParseError(std::ostream& os, const std::string& path,
                         const yactfr::TextParseError& error)
{
    os << internal::formatTextParseError(path, error);
}

std::string escapeStr(const std::string& str)
{
    std::string outStr;

    for (const auto ich : str) {
        const auto uch = static_cast<std::uint8_t>(ich);

        if (uch < 32) {
            switch (ich) {
            case '\a':
                outStr += "\\a";
                break;

            case '\b':
                outStr += "\\b";
                break;

            case '\f':
                outStr += "\\f";
                break;

            case '\n':
                outStr += "\\n";
                break;

            case '\r':
                outStr += "\\r";
                break;

            case '\t':
                outStr += "\\t";
                break;

            case '\v':
                outStr += "\\v";
                break;

            default:
            {
                std::array<char, 8> buf;

                std::sprintf(buf.data(), "\\x%02x", static_cast<unsigned int>(uch));
                outStr += buf.data();
            }
            }
        } else if (ich == '\\') {
            outStr += "\\\\";
        } else if (std::isprint(ich)) {
            outStr += ich;
        } else {
            outStr += '?';
        }
    }

    return outStr;
}

bool isHiddenFile(const bfs::path& path)
{
    return !path.filename().string().empty() && path.filename().string()[0] == '.';
}

bool looksLikeDsFilePath(const bfs::path& path)
{
    if (!path.has_filename()) {
        return false;
    }

    if (path.filename() == "metadata") {
        // metadata stream
        return false;
    }

    if (!bfs::is_regular_file(path)) {
        return false;
    }

    if (isHiddenFile(path)) {
        return false;
    }

    return true;
}

} // namespace utils
} // namespace jacques
