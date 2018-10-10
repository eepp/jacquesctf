/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_SEARCH_PARSER_HPP
#define _JACQUES_SEARCH_PARSER_HPP

#include <memory>
#include <string>
#include <boost/optional.hpp>

#include "utils.hpp"

namespace jacques {

class SearchQuery
{
protected:
    SearchQuery(bool isDiff);

public:
    virtual ~SearchQuery();

    bool isDiff() const
    {
        return _isDiff;
    }

private:
    const bool _isDiff;
};

class SimpleValueSearchQuery :
    public SearchQuery
{
public:
    SimpleValueSearchQuery(bool isDiff, long long value);

    long long value() const
    {
        return _value;
    }

private:
    const long long _value;
};

class PacketIndexSearchQuery :
    public SimpleValueSearchQuery
{
public:
    PacketIndexSearchQuery(bool isDiff, long long value);
};

class EventRecordIndexSearchQuery :
    public SimpleValueSearchQuery
{
public:
    EventRecordIndexSearchQuery(bool isDiff, long long value);
};

class PacketSeqNumSearchQuery :
    public SimpleValueSearchQuery
{
public:
    PacketSeqNumSearchQuery(bool isDiff, long long value);
};

class OffsetSearchQuery :
    public SimpleValueSearchQuery
{
public:
    enum class Unit
    {
        BIT,
        BYTE,
    };

    enum class Target
    {
        PACKET,
        DATA_STREAM_FILE,
    };

public:
    OffsetSearchQuery(bool isDiff, long long value, Unit unit,
                       Target target);

    Unit unit() const
    {
        return _unit;
    }

    Target target() const
    {
        return _target;
    }

private:
    const Unit _unit;
    const Target _target;
};

class TimestampSearchQuery :
    public SimpleValueSearchQuery
{
public:
    enum class Unit
    {
        CYCLE,
        NS,
    };

public:
    TimestampSearchQuery(bool isDiff, long long value, Unit unit);

    Unit unit() const
    {
        return _unit;
    }

private:
    const Unit _unit;
};

class EventRecordTypeNameSearchQuery :
    public SearchQuery
{
public:
    EventRecordTypeNameSearchQuery(std::string&& pattern);

    const std::string& pattern() const
    {
        return _pattern;
    }

    bool matches(const std::string& candidate)
    {
        return utils::globMatch(_pattern, candidate);
    }

private:
    const std::string _pattern;
};

class EventRecordTypeIdSearchQuery :
    public SimpleValueSearchQuery
{
public:
    EventRecordTypeIdSearchQuery(long long value);
};

class SearchParser
{
public:
    SearchParser();
    std::unique_ptr<const SearchQuery> parse(const std::string& input);

private:
    void _skipWhitespaces(std::string::const_iterator& it,
                          std::string::const_iterator end);
    boost::optional<long long> _parseInt(std::string::const_iterator& it,
                                         std::string::const_iterator end);
    std::unique_ptr<const SearchQuery> _parseIndex(std::string::const_iterator& it,
                                                   std::string::const_iterator end,
                                                   bool isDiff, long long mul);
    std::unique_ptr<const SearchQuery> _parseOffset(std::string::const_iterator& it,
                                                    std::string::const_iterator end,
                                                    bool isDiff, long long mul);
    std::unique_ptr<const SearchQuery> _parseTimestamp(std::string::const_iterator& it,
                                                       std::string::const_iterator end,
                                                       bool isDiff, long long mul);
    std::unique_ptr<const SearchQuery> _parseEventRecordTypeId(std::string::const_iterator& it,
                                                               std::string::const_iterator end,
                                                               bool isDiff, long long mul);
    std::unique_ptr<const SearchQuery> _parseEventRecordTypeName(std::string::const_iterator& it,
                                                                 std::string::const_iterator end,
                                                                 bool isDiff, long long mul);
};

} // namespace jacques

#endif // _JACQUES_SEARCH_PARSER_HPP
