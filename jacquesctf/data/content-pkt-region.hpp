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
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <yactfr/metadata/fwd.hpp>

#include "pkt-region.hpp"
#include "scope.hpp"

namespace jacques {

class ContentPktRegion final :
    public PktRegion
{
public:
    using Val = boost::variant<std::int64_t, std::uint64_t, double, std::string>;

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

private:
    void _accept(PktRegionVisitor& visitor) override;

private:
    const yactfr::DataType *_dt;
    boost::optional<Val> _val;
};

} // namespace jacques

#endif // _JACQUES_DATA_CONTENT_PKT_REGION_HPP
