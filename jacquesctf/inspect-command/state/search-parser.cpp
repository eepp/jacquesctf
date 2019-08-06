/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cstdlib>

#include "search-parser.hpp"

namespace jacques {

SearchQuery::SearchQuery(const bool isDiff) :
    _isDiff {isDiff}
{
}

SearchQuery::~SearchQuery()
{
}

SimpleValueSearchQuery::SimpleValueSearchQuery(const bool isDiff,
                                               const long long value) :
    SearchQuery {isDiff},
    _value {value}
{
}

PacketIndexSearchQuery::PacketIndexSearchQuery(const bool isDiff,
                                               const long long value) :
    SimpleValueSearchQuery {isDiff, value}
{
}

EventRecordIndexSearchQuery::EventRecordIndexSearchQuery(const bool isDiff,
                                                         const long long value) :
    SimpleValueSearchQuery {isDiff, value}
{
}

PacketSeqNumSearchQuery::PacketSeqNumSearchQuery(const bool isDiff,
                                                 const long long value) :
    SimpleValueSearchQuery {isDiff, value}
{
}

OffsetSearchQuery::OffsetSearchQuery(const bool isDiff, const long long value,
                                     const Target target) :
    SimpleValueSearchQuery {isDiff, value},
    _target {target}
{
}

TimestampSearchQuery::TimestampSearchQuery(const bool isDiff,
                                           const long long value,
                                           const Unit unit) :
    SimpleValueSearchQuery {isDiff, value},
    _unit {unit}
{
}

EventRecordTypeNameSearchQuery::EventRecordTypeNameSearchQuery(std::string&& pattern) :
    SearchQuery {false},
    _pattern {std::move(pattern)}
{
}

EventRecordTypeIdSearchQuery::EventRecordTypeIdSearchQuery(const long long value) :
    SimpleValueSearchQuery {false, value}
{
}

SearchParser::SearchParser()
{
}

std::unique_ptr<const SearchQuery> SearchParser::parse(const std::string& input)
{
    auto it = std::begin(input);

    // skip initial whitespaces
    this->_skipWhitespaces(it, std::end(input));

    // empty?
    if (it == std::end(input)) {
        return nullptr;
    }

    bool isDiff = false;
    long long mul = 1;

    // differential search?
    if (*it == '+' || *it == '-') {
        isDiff = true;

        if (*it == '-') {
            mul = -1LL;
        }

        ++it;
    }

    if (it == std::end(input)) {
        return nullptr;
    }

    std::unique_ptr<const SearchQuery> ret;

    switch (*it) {
    case '#':
    case '@':
        ret = this->_parseIndex(it, std::end(input), isDiff, mul);
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
        ret = this->_parseOffset(it, std::end(input), isDiff, mul);
        break;

    case '*':
        ret = this->_parseTimestamp(it, std::end(input), isDiff, mul);
        break;

    case '%':
        ret = this->_parseEventRecordTypeId(it, std::end(input), isDiff, mul);
        break;

    case '/':
        ret = this->_parseEventRecordTypeName(it, std::end(input), isDiff, mul);
        break;

    default:
        return nullptr;
    }

    if (!ret) {
        // no specific parsed action
        return nullptr;
    }

    // skip trailing whitespaces
    this->_skipWhitespaces(it, std::end(input));

    if (it != std::end(input)) {
        // non-whitespace garbage at the end
        return nullptr;
    }

    return ret;
}

void SearchParser::_skipWhitespaces(std::string::const_iterator& it,
                                    std::string::const_iterator end)
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

boost::optional<long long> SearchParser::_parseInt(std::string::const_iterator& begin,
                                                   std::string::const_iterator end)
{
    if (begin == end) {
        return boost::none;
    }

    if (*begin < '0' || *begin > '9') {
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
        rBegin = std::cbegin(buf);
    }

    char *strEnd;
    const auto value = std::strtoll(&(*rBegin), &strEnd, 0);

    if ((value == 0 && &(*rBegin) == strEnd) || errno == ERANGE) {
        return boost::none;
    }

    begin += (strEnd - &(*rBegin)) + nbCommas;
    return value;
}

std::unique_ptr<const SearchQuery> SearchParser::_parseIndex(std::string::const_iterator& it,
                                                             std::string::const_iterator end,
                                                             const bool isDiff,
                                                             const long long mul)
{
    if (*it == '@') {
        ++it;

        if (it == end) {
            return nullptr;
        }

        // event record index
        const auto value = this->_parseInt(it, end);

        if (!value) {
            return nullptr;
        }

        return std::make_unique<const EventRecordIndexSearchQuery>(isDiff, mul * *value);
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
        const auto value = this->_parseInt(it, end);

        if (!value) {
            return nullptr;
        }

        return std::make_unique<const PacketSeqNumSearchQuery>(isDiff, mul * *value);
    }

    // packet index
    const auto value = this->_parseInt(it, end);

    if (!value) {
        return nullptr;
    }

    return std::make_unique<const PacketIndexSearchQuery>(isDiff, mul * *value);
}

std::unique_ptr<const SearchQuery> SearchParser::_parseOffset(std::string::const_iterator& it,
                                                              std::string::const_iterator end,
                                                              const bool isDiff,
                                                              const long long mul)
{
    if (it == end) {
        return nullptr;
    }

    OffsetSearchQuery::Target target = OffsetSearchQuery::Target::PACKET;

    if (*it == ':') {
        target = OffsetSearchQuery::Target::DATA_STREAM_FILE;
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

    auto value = this->_parseInt(it, end);

    if (!value) {
        return nullptr;
    }

    if (unitIsBytes) {
        *value *= 8;
    }

    return std::make_unique<const OffsetSearchQuery>(isDiff, mul * *value,
                                                     target);
}

std::unique_ptr<const SearchQuery> SearchParser::_parseTimestamp(std::string::const_iterator& it,
                                                                 std::string::const_iterator end,
                                                                 const bool isDiff,
                                                                 const long long mul)
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

    const auto value = this->_parseInt(it, end);

    if (!value) {
        return nullptr;
    }

    return std::make_unique<const TimestampSearchQuery>(isDiff, mul * *value,
                                                         unit);
}

std::unique_ptr<const SearchQuery> SearchParser::_parseEventRecordTypeId(std::string::const_iterator& it,
                                                                         std::string::const_iterator end,
                                                                         const bool isDiff,
                                                                         const long long mul)
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

    const auto value = this->_parseInt(it, end);

    if (!value) {
        return nullptr;
    }

    return std::make_unique<const EventRecordTypeIdSearchQuery>(*value);
}

std::unique_ptr<const SearchQuery> SearchParser::_parseEventRecordTypeName(std::string::const_iterator& it,
                                                                           std::string::const_iterator end,
                                                                           const bool isDiff,
                                                                           const long long mul)
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
    return std::make_unique<const EventRecordTypeNameSearchQuery>(std::move(pattern));
}

} // namespace jacques
