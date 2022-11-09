/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMON_SEARCH_QUERY_HPP
#define _JACQUES_INSPECT_COMMON_SEARCH_QUERY_HPP

#include <memory>
#include <string>
#include <boost/optional.hpp>

#include "utils.hpp"

namespace jacques {

class SearchQuery
{
protected:
    SearchQuery(bool isDiff) noexcept;

public:
    virtual ~SearchQuery() = default;

    bool isDiff() const noexcept
    {
        return _isDiff;
    }

private:
    const bool _isDiff;
};

class SimpleValSearchQuery :
    public SearchQuery
{
protected:
    explicit SimpleValSearchQuery(bool isDiff, long long val) noexcept;

public:
    long long val() const noexcept
    {
        return _val;
    }

private:
    const long long _val;
};

class PktIndexSearchQuery final :
    public SimpleValSearchQuery
{
public:
    explicit PktIndexSearchQuery(bool isDiff, long long val) noexcept;
};

class PktSeqNumSearchQuery final :
    public SimpleValSearchQuery
{
public:
    explicit PktSeqNumSearchQuery(bool isDiff, long long val) noexcept;
};

class ErIndexSearchQuery final :
    public SimpleValSearchQuery
{
public:
    explicit ErIndexSearchQuery(bool isDiff, long long val) noexcept;
};

class OffsetSearchQuery final :
    public SimpleValSearchQuery
{
public:
    enum class Target
    {
        PKT,
        DS_FILE,
    };

public:
    explicit OffsetSearchQuery(bool isDiff, long long val, Target target) noexcept;

    Target target() const noexcept
    {
        return _target;
    }

private:
    const Target _target;
};

class TimestampSearchQuery final :
    public SimpleValSearchQuery
{
public:
    enum class Unit
    {
        CYCLE,
        NS,
    };

public:
    explicit TimestampSearchQuery(bool isDiff, long long val, Unit unit) noexcept;

    Unit unit() const noexcept
    {
        return _unit;
    }

private:
    const Unit _unit;
};

class ErtNameSearchQuery final :
    public SearchQuery
{
public:
    explicit ErtNameSearchQuery(std::string pattern);

    const std::string& pattern() const noexcept
    {
        return _pattern;
    }

    bool matches(const std::string& candidate) const noexcept
    {
        return utils::globMatch(_pattern, candidate);
    }

private:
    const std::string _pattern;
};

class ErtIdSearchQuery final :
    public SimpleValSearchQuery
{
public:
    explicit ErtIdSearchQuery(long long val) noexcept;
};

std::unique_ptr<const SearchQuery> parseSearchQuery(const std::string& input);

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMON_SEARCH_QUERY_HPP
