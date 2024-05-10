/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_UTILS_HPP
#define _JACQUES_UTILS_HPP

#include <string>
#include <iostream>
#include <utility>
#include <sstream>
#include <iterator>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <yactfr/yactfr.hpp>

#include "data/metadata-error.hpp"
#include "io-error.hpp"
#include "cmd-error.hpp"
#include "aliases.hpp"
#include "cfg.hpp"

namespace jacques {
namespace utils {

/*
 * Formats the path `path`, considering that the total length of the
 * output must not exceed `maxLen`.
 *
 * The first element of the returned pair is the directory name
 * (possibly with ellipses if, due to `maxLen`, it's incomplete), and
 * the second element is the file name.
 *
 * The lengths of both returned elements plus one (to add a path
 * separator between them) is always less than or equal to `maxLen`.
 */
std::pair<std::string, std::string> formatPath(const boost::filesystem::path& path, Size maxLen);

/*
 * Normalizes a globbing pattern, that is, removes consecutive `*`
 * characters.
 */
std::string normalizeGlobPattern(const std::string& pattern);

/*
 * Returns whether or not `candidate` matches the globbing pattern
 * `pattern`. Only the `*` special character works as of this version.
 */
bool globMatch(const std::string& pattern, const std::string& candidate);

/*
 * Does pretty much what the fold(1) command does.
 */
std::string wrapText(const std::string& text, Size lineLen);

/*
 * Does pretty much what the fold(1) command does.
 */
std::vector<std::string> wrapTextLines(const std::string& text, Size lineLen);

/*
 * Escapes a string, replacing special characters with typical escape
 * sequences.
 */
std::string escapeStr(const std::string& str);

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
                                              const boost::optional<char>& sep = boost::none);

std::pair<std::string, std::string> formatNs(long long ns,
                                             const boost::optional<char>& sep = boost::none);

void printTextParseError(std::ostream& os, const std::string& path,
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

static inline std::string formatTextParseError(const std::string& path,
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
boost::optional<std::string> tryFunc(FuncT&& func)
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
        return boost::none;
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

    const auto fmtStr = [withBackticks](const auto& str) {
        if (withBackticks) {
            return std::string {'`'} + str + '`';
        }

        return str;
    };

    if (container.size() == 1) {
        return fmtStr(*container.begin());
    }

    if (container.size() == 2 && lastWord) {
        std::ostringstream ss;

        ss << fmtStr(*container.begin());
        ss << ' ' << lastWord << ' ';
        ss << fmtStr(*std::next(container.begin()));
        return ss.str();
    }

    std::ostringstream ss;
    const auto penultimateIt = std::prev(container.end());

    for (auto it = container.begin(); it < container.end(); ++it) {
        if (it == penultimateIt && lastWord) {
            ss << lastWord << ' ';
        }

        ss << fmtStr(*it);

        if (it != penultimateIt) {
            ss << ", ";
        }
    }

    return ss.str();
}

/*
 * Partial implementation of INVOKE.
 *
 * As found in
 * <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0312r1.html>.
 */
template <typename FuncT, typename... ArgTs>
auto call(FuncT func, ArgTs&&...args) -> decltype(std::ref(func)(std::forward<ArgTs>(args)...))
{
    return std::ref(func)(std::forward<ArgTs>(args)...);
}

} // namespace utils
} // namespace jacques

#endif // _JACQUES_UTILS_HPP
