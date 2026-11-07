/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_UTILS_HPP
#define _JACQUES_UTILS_HPP

#include <string>
#include <string_view>
#include <ostream>
#include <utility>
#include <iterator>
#include <optional>
#include <boost/filesystem.hpp>
#include <fmt/format.h>
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
 * Prints the error message `msg`, prefixed with `ERROR: ` and followed
 * with a newline, to the standard error.
 */
void printError(std::string_view msg);

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
    std::string str;

    for (auto it = error.messages().rbegin(); it != error.messages().rend(); ++it) {
        auto& msg = *it;

        fmt::format_to(std::back_inserter(str), "{}:{}:{} [{}]: {}", path,
                       msg.location().naturalLineNumber(), msg.location().naturalColumnNumber(),
                       msg.location().offset(), msg.message());

        if (it < error.messages().rend() - 1) {
            str += '\n';
        }
    }

    return str;
}

} // namespace internal

template <typename FuncT>
std::optional<std::string> tryFunc(FuncT&& func)
{
    std::string str;

    try {
        func();
    } catch (const MetadataError<yactfr::InvalidMetadataStream>& exc) {
        str = fmt::format("Metadata error: `{}`: invalid metadata stream: {}",
                          exc.path().string(), exc.what());
    } catch (const MetadataError<yactfr::TextParseError>& exc) {
        str = internal::formatTextParseError(exc.path().string(), exc.subError());
    } catch (const CliError& exc) {
        str = fmt::format("Command-line error: {}", exc.what());
    } catch (const boost::filesystem::filesystem_error& exc) {
        str = fmt::format("File system error: {}", exc.what());
    } catch (const IOError& exc) {
        str = fmt::format("I/O error: {}", exc.what());
    } catch (const CmdError& exc) {
        str = fmt::format("Command error: {}", exc.what());
    } catch (const std::exception& exc) {
        str = exc.what();
    } catch (...) {
        str = "Unknown exception";
    }

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
            return fmt::format("`{}`", str);
        }

        return str;
    };

    if (container.size() == 1) {
        return fmtStrFunc(*container.begin());
    }

    if (container.size() == 2 && lastWord) {
        return fmt::format("{} {} {}", fmtStrFunc(*container.begin()), lastWord,
                           fmtStrFunc(*std::next(container.begin())));
    }

    std::string str;
    const auto penultimateIt = std::prev(container.end());

    for (auto it = container.begin(); it < container.end(); ++it) {
        if (it == penultimateIt && lastWord) {
            fmt::format_to(std::back_inserter(str), "{} ", lastWord);
        }

        str += fmtStrFunc(*it);

        if (it != penultimateIt) {
            str += ", ";
        }
    }

    return str;
}

} // namespace jacques::utils

#endif // _JACQUES_UTILS_HPP
