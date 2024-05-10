/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "content-pkt-region.hpp"

namespace jacques {
namespace {

OptBo boFromDt(const yactfr::DataType& dt)
{
    if (dt.isFixedLengthBitArrayType()) {
        return dt.asFixedLengthBitArrayType().byteOrder();
    }

    return boost::none;
}

} // namespace

ContentPktRegion::ContentPktRegion(const PktSegment& segment, Scope::SP scope,
                                   const yactfr::DataType& dt, boost::optional<Val> val) noexcept :
    PktRegion {
        segment,
        std::move(scope)
    },
    _dt {&dt},
    _val {std::move(val)}
{
    this->_segment().bo(boFromDt(dt));
}

std::set<std::string> ContentPktRegion::flagOrMappingNames() const
{
    assert(_val);

    std::unordered_set<const std::string *> names;

    if (_dt->isFixedLengthBitMapType()) {
        _dt->asFixedLengthBitMapType().activeFlagNamesForUnsignedIntegerValue(boost::get<unsigned long long>(*_val),
                                                                              names);
    } else if (_dt->isFixedLengthUnsignedIntegerType()) {
        names = this->_mappingNamesOfVal(_dt->asFixedLengthUnsignedIntegerType());
    } else if (_dt->isFixedLengthSignedIntegerType()) {
        names = this->_mappingNamesOfVal(_dt->asFixedLengthSignedIntegerType());
    } else if (_dt->isVariableLengthUnsignedIntegerType()) {
        names = this->_mappingNamesOfVal(_dt->asVariableLengthUnsignedIntegerType());
    } else if (_dt->isVariableLengthSignedIntegerType()) {
        names = this->_mappingNamesOfVal(_dt->asVariableLengthSignedIntegerType());
    }

    std::set<std::string> sortedNames;

    for (const auto namePtr : names) {
        sortedNames.insert(*namePtr);
    }

    return sortedNames;
}

void ContentPktRegion::_accept(PktRegionVisitor& visitor)
{
    visitor.visit(*this);
}

} // namespace jacques
