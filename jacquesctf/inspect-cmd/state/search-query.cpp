/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cstdlib>
#include <cctype>

#include "search-query.hpp"

namespace jacques {

SearchQuery::SearchQuery(const bool isDiff) noexcept :
    _isDiff {isDiff}
{
}

SimpleValSearchQuery::SimpleValSearchQuery(const bool isDiff, const long long val) noexcept :
    SearchQuery {isDiff},
    _val {val}
{
}

PktIndexSearchQuery::PktIndexSearchQuery(const bool isDiff, const long long val) noexcept :
    SimpleValSearchQuery {isDiff, val}
{
}

ErIndexSearchQuery::ErIndexSearchQuery(const bool isDiff, const long long val) noexcept :
    SimpleValSearchQuery {isDiff, val}
{
}

PktSeqNumSearchQuery::PktSeqNumSearchQuery(const bool isDiff, const long long val) noexcept :
    SimpleValSearchQuery {isDiff, val}
{
}

OffsetSearchQuery::OffsetSearchQuery(const bool isDiff, const long long val, const Target target) noexcept :
    SimpleValSearchQuery {isDiff, val},
    _target {target}
{
}

TimestampSearchQuery::TimestampSearchQuery(const bool isDiff, const long long val, const Unit unit) noexcept :
    SimpleValSearchQuery {isDiff, val},
    _unit {unit}
{
}

ErtNameSearchQuery::ErtNameSearchQuery(std::string pattern) :
    SearchQuery {false},
    _pattern {std::move(pattern)}
{
}

ErtIdSearchQuery::ErtIdSearchQuery(const long long val) noexcept :
    SimpleValSearchQuery {false, val}
{
}

static void skipWs(std::string::const_iterator& it, std::string::const_iterator end)
{
    while (it != end) {
        if (*it == ' ' || *it == '\t' || *it == '\n' ||
                *it == '\r' || *it == '\v') {
            ++it;
            continue;
        }

        break;
    }
}

static boost::optional<long long> parseInt(std::string::const_iterator& begin,
                                           std::string::const_iterator end)
{
    if (begin == end) {
        return boost::none;
    }

    if (!std::isdigit(*begin)) {
        return boost::none;
    }

    auto rBegin = begin;
    std::string buf;
    auto nbCommas = 0U;

    if (*begin != '0') {
        // remove commas
        for (auto it = begin; it != end; ++it) {
            if (*it == ',') {
                ++nbCommas;
                continue;
            }

            if (!std::isdigit(*it)) {
                break;
            }

            buf += *it;
        }

        assert(!buf.empty());
        rBegin = buf.begin();
    }

    char *strEnd;
    const auto val = std::strtoll(&(*rBegin), &strEnd, 0);

    if ((val == 0 && &(*rBegin) == strEnd) || errno == ERANGE) {
        return boost::none;
    }

    begin += (strEnd - &(*rBegin)) + nbCommas;
    return val;
}

static std::unique_ptr<const SearchQuery> parseIndex(std::string::const_iterator& it,
                                                     std::string::const_iterator end,
                                                     const bool isDiff, const long long mul)
{
    if (*it == '@') {
        ++it;

        if (it == end) {
            return nullptr;
        }

        // event record index
        const auto val = parseInt(it, end);

        if (!val) {
            return nullptr;
        }

        return std::make_unique<const ErIndexSearchQuery>(isDiff, *val * mul);
    }

    ++it;

    if (it == end) {
        return nullptr;
    }

    if (*it == '#') {
        ++it;

        if (it == end) {
            return nullptr;
        }

        // packet sequence number
        const auto val = parseInt(it, end);

        if (!val) {
            return nullptr;
        }

        return std::make_unique<const PktSeqNumSearchQuery>(isDiff, *val * mul);
    }

    // packet index
    const auto val = parseInt(it, end);

    if (!val) {
        return nullptr;
    }

    return std::make_unique<const PktIndexSearchQuery>(isDiff, *val * mul);
}

static std::unique_ptr<const SearchQuery> parseOffset(std::string::const_iterator& it,
                                                      std::string::const_iterator end,
                                                      const bool isDiff, const long long mul)
{
    if (it == end) {
        return nullptr;
    }

    OffsetSearchQuery::Target target = OffsetSearchQuery::Target::PKT;

    if (*it == ':') {
        target = OffsetSearchQuery::Target::DS_FILE;
        ++it;

        if (it == end) {
            return nullptr;
        }
    }

    bool unitIsBytes = false;

    if (*it == '$') {
        unitIsBytes = true;
        ++it;

        if (it == end) {
            return nullptr;
        }
    }

    auto val = parseInt(it, end);

    if (!val) {
        return nullptr;
    }

    if (unitIsBytes) {
        *val *= 8;
    }

    return std::make_unique<const OffsetSearchQuery>(isDiff, *val * mul, target);
}

static std::unique_ptr<const SearchQuery> parseTs(std::string::const_iterator& it,
                                                  std::string::const_iterator end,
                                                  const bool isDiff, const long long mul)
{
    if (it == end) {
        return nullptr;
    }

    TimestampSearchQuery::Unit unit = TimestampSearchQuery::Unit::NS;

    // skip '*'
    ++it;

    if (it == end) {
        return nullptr;
    }

    if (*it == '*') {
        unit = TimestampSearchQuery::Unit::CYCLE;
        ++it;

        if (it == end) {
            return nullptr;
        }
    }

    const auto val = parseInt(it, end);

    if (!val) {
        return nullptr;
    }

    return std::make_unique<const TimestampSearchQuery>(isDiff, *val * mul, unit);
}

static std::unique_ptr<const SearchQuery> parseErtId(std::string::const_iterator& it,
                                                     std::string::const_iterator end,
                                                     const bool isDiff, const long long mul)
{
    if (it == end) {
        return nullptr;
    }

    if (isDiff) {
        return nullptr;
    }

    // skip '%'
    ++it;

    if (it == end) {
        return nullptr;
    }

    const auto val = parseInt(it, end);

    if (!val) {
        return nullptr;
    }

    return std::make_unique<const ErtIdSearchQuery>(*val);
}

static std::unique_ptr<const SearchQuery> parseErtName(std::string::const_iterator& it,
                                                       std::string::const_iterator end,
                                                       const bool isDiff, const long long mul)
{
    if (it == end) {
        return nullptr;
    }

    if (isDiff) {
        return nullptr;
    }

    // skip '/'
    ++it;

    if (it == end) {
        return nullptr;
    }

    std::string pattern {&(*it), static_cast<std::string::size_type>(end - it)};

    it = end;

    // normalize pattern (no consecutive `*`)
    utils::normalizeGlobPattern(pattern);
    return std::make_unique<const ErtNameSearchQuery>(std::move(pattern));
}

std::unique_ptr<const SearchQuery> parseSearchQuery(const std::string& input)
{
    auto it = input.begin();

    // skip initial whitespaces
    skipWs(it, input.end());

    // empty?
    if (it == input.end()) {
        return nullptr;
    }

    bool isDiff = false;
    auto mul = 1LL;

    // differential search?
    if (*it == '+' || *it == '-') {
        isDiff = true;

        if (*it == '-') {
            mul = -1LL;
        }

        ++it;
    }

    if (it == input.end()) {
        return nullptr;
    }

    std::unique_ptr<const SearchQuery> ret;

    switch (*it) {
    case '#':
    case '@':
        ret = parseIndex(it, input.end(), isDiff, mul);
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case ':':
    case '$':
        ret = parseOffset(it, input.end(), isDiff, mul);
        break;

    case '*':
        ret = parseTs(it, input.end(), isDiff, mul);
        break;

    case '%':
        ret = parseErtId(it, input.end(), isDiff, mul);
        break;

    case '/':
        ret = parseErtName(it, input.end(), isDiff, mul);
        break;

    default:
        return nullptr;
    }

    if (!ret) {
        // no specific parsed action
        return nullptr;
    }

    // skip trailing whitespaces
    skipWs(it, input.end());

    if (it != input.end()) {
        // non-whitespace garbage at the end
        return nullptr;
    }

    return ret;
}

} // namespace jacques
