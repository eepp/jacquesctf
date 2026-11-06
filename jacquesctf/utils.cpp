/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cwctype>
#include <array>
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>
#include <algorithm>

#include "utils.hpp"
#include "unicode.hpp"
#include "cfg.hpp"

namespace jacques::utils {

namespace bfs = boost::filesystem;

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

std::string format(const char * const fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    va_list argsCopy;

    va_copy(argsCopy, args);

    const auto len = std::vsnprintf(nullptr, 0, fmt, argsCopy);

    va_end(argsCopy);

    std::string str;

    if (len > 0) {
        str.resize(static_cast<std::size_t>(len));
        static_cast<void>(std::vsnprintf(str.data(), str.size() + 1, fmt, args));
    }

    va_end(args);
    return str;
}

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

std::string normalizeGlobPattern(const std::string_view pattern)
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

inline bool atEndOfPattern(const char * const p, const std::string_view pattern) noexcept
{
    return static_cast<Size>(p - pattern.data()) == pattern.size() || *p == '\0';
}

} // namespace

bool globMatch(const std::string_view pattern, const std::string_view candidate)
{
    auto retryC = candidate.data();
    auto retryP = pattern.data();
    const char *c, *p;
    auto gotAStar = false;
    const auto candidateLen = candidate.size();

retry:
    c = retryC;
    p = retryP;

    while (static_cast<Size>(c - candidate.data()) < candidateLen && *c != '\0') {
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
                                              const std::optional<char>& sep)
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

std::pair<std::string, std::string> formatNs(long long ns, const std::optional<char>& sep)
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

void printTextParseError(std::ostream& os, const std::string_view path,
                         const yactfr::TextParseError& error)
{
    os << internal::formatTextParseError(path, error);
}

std::string escapeStr(const std::string_view str)
{
    std::string outStr;
    auto rest = str;

    while (!rest.empty()) {
        const auto cp = unicode::popFrontUtf8(rest);

        if (cp < 32) {
            switch (cp) {
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

                std::sprintf(buf.data(), "\\x%02x", static_cast<unsigned int>(cp));
                outStr += buf.data();
            }
            }
        } else if (cp == U'\\') {
            outStr += "\\\\";
        } else if (std::iswprint(static_cast<std::wint_t>(cp))) {
            unicode::appendUtf8(outStr, cp);
        } else {
            unicode::appendUtf8(outStr, unicode::replacementCh);
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

yactfr::DisplayBase intTypePrefDispBase(const yactfr::DataType& dt) noexcept
{
    assert(dt.isIntegerType());

    if (dt.isFixedLengthUnsignedIntegerType()) {
        return dt.asFixedLengthUnsignedIntegerType().preferredDisplayBase();
    } else if (dt.isFixedLengthSignedIntegerType()) {
        return dt.asFixedLengthSignedIntegerType().preferredDisplayBase();
    } else if (dt.isVariableLengthUnsignedIntegerType()) {
        return dt.asVariableLengthUnsignedIntegerType().preferredDisplayBase();
    } else {
        assert(dt.isVariableLengthSignedIntegerType());
        return dt.asVariableLengthSignedIntegerType().preferredDisplayBase();
    }
}

} // namespace jacques::utils
