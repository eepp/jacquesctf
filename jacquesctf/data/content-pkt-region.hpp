/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_CONTENT_PKT_REGION_HPP
#define _JACQUES_DATA_CONTENT_PKT_REGION_HPP

#include <memory>
#include <cstdint>
#include <set>
#include <string>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <yactfr/yactfr.hpp>

#include "pkt-region.hpp"
#include "scope.hpp"

namespace jacques {

class ContentPktRegion final :
    public PktRegion
{
public:
    using Val = boost::variant<nullptr_t, bool, unsigned long long, long long, double, std::string>;

public:
    explicit ContentPktRegion(const PktSegment& segment, Scope::SP scope,
                              const yactfr::DataType& dt, boost::optional<Val> val) noexcept;

    const yactfr::DataType& dt() const noexcept
    {
        return *_dt;
    }

    const boost::optional<Val>& val() const noexcept
    {
        return _val;
    }

    /*
     * Returns the sorted active flag or mapping names of this packet
     * region.
     *
     * val() must have a value.
     *
     * If dt() doesn't return a bit map type or an integer type, then
     * this method returns an empty set.
     */
    std::set<std::string> flagOrMappingNames() const;

private:
    void _accept(PktRegionVisitor& visitor) override;

    template <typename IntTypeT>
    std::unordered_set<const std::string *> _mappingNamesOfVal(const IntTypeT& intType) const
    {
        std::unordered_set<const std::string *> names;

        intType.mappingNamesForValue(boost::get<typename IntTypeT::MappingValue>(*_val), names);
        return names;
    }

private:
    const yactfr::DataType *_dt;
    boost::optional<Val> _val;
};

} // namespace jacques

#endif // _JACQUES_DATA_CONTENT_PKT_REGION_HPP
