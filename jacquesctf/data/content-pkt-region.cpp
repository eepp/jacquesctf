/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "content-pkt-region.hpp"
#include "aliases.hpp"
#include "utils.hpp"

namespace jacques {
namespace {

OptBo boFromDt(const yactfr::DataType& dt)
{
    if (dt.isFixedLengthBitArrayType()) {
        return dt.asFixedLengthBitArrayType().byteOrder();
    }

    return boost::none;
}

#ifndef NDEBUG
bool arrayIndexesLenIsExpected(const DtPath& dtPath,
                               const ContentPktRegion::ArrayIndexes& arrayIndexes)
{
    return utils::call([&dtPath] {
        Size count = 0;

        for (auto& item : dtPath.items()) {
            if (boost::get<DtPath::CurArrayElemItem>(&item)) {
                ++count;
            }
        }

        return count;
    }) == arrayIndexes.size();
}
#endif

} // namespace

ContentPktRegion::ContentPktRegion(const PktSegment& segment, Scope::SP scope,
                                   const yactfr::DataType& dt, const DtPath& dtPath,
                                   ArrayIndexes arrayIndexes, boost::optional<Val> val) noexcept :
    PktRegion {
        segment,
        std::move(scope)
    },
    _dt {&dt},
    _dtPath {&dtPath},
    _arrayIndexes {std::move(arrayIndexes)},
    _val {std::move(val)}
{
    assert(arrayIndexesLenIsExpected(dtPath, _arrayIndexes));
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
