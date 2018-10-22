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
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"

// used to mark a variable as being unused
#define JACQUES_UNUSED(_x)  ((void) _x)

namespace jacques {
namespace utils {

/*
 * Formats the path `path`, considering that the output's total length
 * must not exceed `maxLen`. The pair's first element is the directory
 * name (possibly with ellipses if, due to `maxLen`, it's incomplete),
 * and the second element is the file name. The lengths of both elements
 * plus one (to add a path separator between them) is always lesser than
 * or equal to `maxLen`.
 */
std::pair<std::string, std::string> formatPath(const boost::filesystem::path& path,
                                               Size maxLen);

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
std::string wrapText(const std::string& text, Size lineLength);

/*
 * Creates a string which has "thousands separators" from a value,
 * like so:
 *
 *     1827912   -> 1 827 912
 *     -21843812 -> -21,843,812
 */
std::string sepNumber(long long value, char sep = ' ');
std::string sepNumber(unsigned long long value, char sep = ' ');

/*
 * Used as such:
 *
 *     error() << "Cannot something something" << std::endl;
 */
static inline std::ostream& error()
{
    std::cerr << "ERROR: ";
    return std::cerr;
}

enum class SizeFormatMode
{
    FULL_FLOOR,
    FULL_FLOOR_WITH_EXTRA_BITS,
    BYTES_FLOOR,
    BYTES_FLOOR_WITH_EXTRA_BITS,
    BITS,
};

std::pair<std::string, std::string> formatSize(Size sizeBits,
                                               SizeFormatMode formatMode = SizeFormatMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                               const boost::optional<char>& sep = boost::none);

std::pair<std::string, std::string> formatNs(long long ns,
                                             const boost::optional<char>& sep = boost::none);

} // namespace utils
} // namespace jacques

#endif // _JACQUES_UTILS_HPP
