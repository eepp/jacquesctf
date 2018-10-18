/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DURATION_HPP
#define _JACQUES_DURATION_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <boost/operators.hpp>
#include <yactfr/element.hpp>
#include <yactfr/metadata/clock-type.hpp>

#include "aliases.hpp"

namespace jacques {

class Duration :
    public boost::totally_ordered<Duration>
{
public:
    struct Parts
    {
        unsigned long long hours;
        unsigned long long minutes;
        unsigned long long seconds;
        unsigned long long ns;
    };

public:
    Duration() = default;
    explicit Duration(unsigned long long ns);
    Duration(const Duration&) = default;
    Duration& operator=(const Duration&) = default;
    Parts parts() const;
    void format(char *buf, Size bufSize) const;
    std::string format() const;

    unsigned long long ns() const
    {
        return _ns;
    }

    bool operator==(const Duration& other) const noexcept
    {
        return _ns - other._ns;
    }

    bool operator<(const Duration& other) const noexcept
    {
        return _ns < other._ns;
    }

private:
    unsigned long long _ns = 0;
};

static inline
std::ostream& operator<<(std::ostream& stream, const Duration& duration)
{
    std::array<char, 64> buf;

    duration.format(buf.data(), buf.size());
    stream << buf.data();
    return stream;
}

} // namespace jacques

#endif // _JACQUES_DURATION_HPP
