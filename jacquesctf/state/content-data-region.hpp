/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_CONTENT_DATA_REGION_HPP
#define _JACQUES_CONTENT_DATA_REGION_HPP

#include <memory>
#include <cstdint>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "data-region.hpp"
#include "scope.hpp"

namespace jacques {

class ContentDataRegion :
    public DataRegion
{
public:
    using Value = boost::variant<std::int64_t,
                                 std::uint64_t,
                                 double,
                                 std::string>;

public:
    explicit ContentDataRegion(const DataSegment& segment, Data&& data,
                               Scope::SP scope,
                               const yactfr::DataType& dataType,
                               const boost::optional<Value>& value);

    const Scope& scope() const noexcept
    {
        return *_scope;
    }

    Scope::SP scopePtr() const
    {
        return _scope;
    }

    const yactfr::DataType& dataType() const noexcept
    {
        return *_dataType;
    }

    const boost::optional<Value>& value() const noexcept
    {
        return _value;
    }

private:
    Scope::SP _scope;
    const yactfr::DataType *_dataType;
    boost::optional<Value> _value;
};

} // namespace jacques

#endif // _JACQUES_CONTENT_DATA_REGION_HPP
