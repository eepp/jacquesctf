/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_UTILS_HPP
#define _JACQUES_UTILS_HPP

#include <string>
#include <string_view>
#include <iostream>
#include <utility>
#include <sstream>
#include <iterator>
#include <optional>
#include <boost/filesystem.hpp>
#include <yactfr/yactfr.hpp>

#include "data/metadata-error.hpp"
#include "io-error.hpp"
#include "cmd-error.hpp"
#include "aliases.hpp"
#include "cfg.hpp"

namespace jacques::utils {

/*
 * Normalizes a globbing pattern, that is, removes consecutive `*`
 * characters.
 */
[[ nodiscard ]] std::string normalizeGlobPattern(std::string_view pattern);

/*
 * Returns whether or not `candidate` matches the globbing pattern
 * `pattern`. Only the `*` special character works as of this version.
 */
bool globMatch(std::string_view pattern, std::string_view candidate);

/*
 * Does pretty much what the fold(1) command does.
 */
std::string wrapText(const std::string& text, Size lineLen);

/*
 * Does pretty much what the fold(1) command does.
 */
std::vector<std::string> wrapTextLines(const std::string& text, Size lineLen);

/*
 * Escapes the UTF-8 string `str`, replacing control characters with
 * typical escape sequences.
 *
 * The returned string is valid UTF-8: this function replaces invalid
 * sequences and characters which aren't printable according to the
 * current locale (see std::iswprint()) with `unicode::replacementCh`.
 */
std::string escapeStr(std::string_view str);

/*
 * Formats a string like std::printf() does.
 */
[[ nodiscard ]] std::string format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/*
 * Creates a string which has "thousands separators" from a value,
 * like so:
 *
 *     1827912   -> 1 827 912
 *     -21843812 -> -21,843,812
 */
std::string sepNumber(long long val, char sep = ' ');
std::string sepNumber(unsigned long long val, char sep = ' ');

/*
 * Returns whether or not the path `path` identifies a hidden
 * file/directory.
 */
bool isHiddenFile(const boost::filesystem::path& path);

/*
 * Returns whether or not the path `path` looks like a CTF data stream
 * file path, only considering the path itself.
 */
bool looksLikeDsFilePath(const boost::filesystem::path& path);

/*
 * Returns the preferred display base of the integer type `dt`.
 */
yactfr::DisplayBase intTypePrefDispBase(const yactfr::DataType& dt) noexcept;

/*
 * Used as such:
 *
 *     error() << "Cannot something something" << std::endl;
 *
 * I hate this. Let's change it some day.
 */
static inline std::ostream& error()
{
    std::cerr << "ERROR: ";
    return std::cerr;
}

enum class LenFmtMode
{
    FULL_FLOOR,
    FULL_FLOOR_WITH_EXTRA_BITS,
    BYTES_FLOOR,
    BYTES_FLOOR_WITH_EXTRA_BITS,
    BITS,
};

std::pair<std::string, std::string> formatLen(Size lenBits,
                                              LenFmtMode fmtMode = LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                              const std::optional<char>& sep = std::nullopt);

std::pair<std::string, std::string> formatNs(long long ns,
                                             const std::optional<char>& sep = std::nullopt);

void printTextParseError(std::ostream& os, std::string_view path,
                         const yactfr::TextParseError& error);

namespace internal {

static inline void maybeAppendPeriod(std::string& str)
{
    if (str.empty()) {
        return;
    }

    if (str.back() != '.') {
        str += '.';
    }
}

static inline std::string formatTextParseError(const std::string_view path,
                                               const yactfr::TextParseError& error)
{
    std::ostringstream ss;

    for (auto it = error.messages().rbegin(); it != error.messages().rend(); ++it) {
        auto& msg = *it;

        ss << path << ":" << msg.location().naturalLineNumber() <<
              ":" << msg.location().naturalColumnNumber() << " [" <<
              msg.location().offset() << "]: " << msg.message();

        if (it < error.messages().rend() - 1) {
            ss << std::endl;
        }
    }

    return ss.str();
}

} // namespace internal

template <typename FuncT>
std::optional<std::string> tryFunc(FuncT&& func)
{
    std::ostringstream ss;

    try {
        func();
    } catch (const MetadataError<yactfr::InvalidMetadataStream>& exc) {
        ss << "Metadata error: `" << exc.path().string() <<
              "`: invalid metadata stream: " << exc.what();
    } catch (const MetadataError<yactfr::TextParseError>& exc) {
        ss << internal::formatTextParseError(exc.path().string(), exc.subError());
    } catch (const CliError& exc) {
        ss << "Command-line error: " << exc.what();
    } catch (const boost::filesystem::filesystem_error& exc) {
        ss << "File system error: " << exc.what();
    } catch (const IOError& exc) {
        ss << "I/O error: " << exc.what();
    } catch (const CmdError& exc) {
        ss << "Command error: " << exc.what();
    } catch (const std::exception& exc) {
        ss << exc.what();
    } catch (...) {
        ss << "Unknown exception";
    }

    auto str = ss.str();

    if (str.empty()) {
        return std::nullopt;
    }

    internal::maybeAppendPeriod(str);
    return str;
}

template <typename ContainerT>
std::string csvListStr(const ContainerT& container, const bool withBackticks = true,
                       const char * const lastWord = "and")
{
    if (container.empty()) {
        return "";
    }

    const auto fmtStrFunc = [withBackticks](const auto& str) {
        if (withBackticks) {
            return std::string {'`'} + str + '`';
        }

        return str;
    };

    if (container.size() == 1) {
        return fmtStrFunc(*container.begin());
    }

    if (container.size() == 2 && lastWord) {
        std::ostringstream ss;

        ss << fmtStrFunc(*container.begin());
        ss << ' ' << lastWord << ' ';
        ss << fmtStrFunc(*std::next(container.begin()));
        return ss.str();
    }

    std::ostringstream ss;
    const auto penultimateIt = std::prev(container.end());

    for (auto it = container.begin(); it < container.end(); ++it) {
        if (it == penultimateIt && lastWord) {
            ss << lastWord << ' ';
        }

        ss << fmtStrFunc(*it);

        if (it != penultimateIt) {
            ss << ", ";
        }
    }

    return ss.str();
}

} // namespace jacques::utils

#endif // _JACQUES_UTILS_HPP
