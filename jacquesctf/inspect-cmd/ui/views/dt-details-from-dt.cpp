/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <array>
#include <cstring>
#include <yactfr/yactfr.hpp>

#include "dt-details-from-dt.hpp"
#include "abstract-dt-details.hpp"
#include "dt-details.hpp"
#include "enum-type-mapping-details.hpp"

namespace jacques {

static void _dtDetailsFromDt(const yactfr::DataType& dt, Size indent, const Stylist& stylist,
                             const boost::optional<std::string>& name, Size nameWidth,
                             boost::optional<std::string> extra, Size extraWidth,
                             std::vector<std::unique_ptr<const AbstractDtDetails>>& vec);

template <typename EnumTypeT>
static void _enumTypeMappingDetailsFromDt(const EnumTypeT& dt, const Size indent,
                                          const Stylist& stylist,
                                          std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    for (auto& nameRangesPair : dt.mappings()) {
        auto& name = nameRangesPair.first;
        auto rangesStr = intRangeSetStr(nameRangesPair.second);

        vec.push_back(std::make_unique<const EnumTypeMappingDetails>(name, std::move(rangesStr),
                                                                     indent, stylist));
    }
}

template <typename VarTypeOptT>
Size _varTypeOptDispNameSize(const VarTypeOptT& opt) noexcept
{
    return opt.displayName() ? opt.displayName()->size() : 0;
}

template <typename VarTypeT>
void _dtDetailsFromVarType(const VarTypeT& varType, const Size indent, const Stylist& stylist,
                           std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    if (varType.isEmpty()) {
        return;
    }

    const auto maxDispNameSizeIt = std::max_element(varType.begin(), varType.end(),
                                                    [](auto& optA, auto& optB) {
        return _varTypeOptDispNameSize(*optA) < _varTypeOptDispNameSize(*optB);
    });

    std::vector<std::string> rangesStrs;

    for (auto& opt : varType) {
        rangesStrs.push_back(intRangeSetStr(opt->selectorRanges()));
    }

    const auto maxRangesStrSizeIt = std::max_element(rangesStrs.cbegin(), rangesStrs.cend(),
                                                     [](auto& strA, auto& strB) {
        return strA.size() < strB.size();
    });

    const auto extraWidth = maxRangesStrSizeIt->size();

    for (Index i = 0; i < varType.size(); ++i) {
        auto& opt = varType[i];
        auto& rangesStr = rangesStrs[i];

        _dtDetailsFromDt(opt.dataType(), indent + 2, stylist, opt.displayName(),
                         _varTypeOptDispNameSize(**maxDispNameSizeIt), std::move(rangesStr),
                         extraWidth, vec);
    }
}

static void _dtDetailsFromOptType(const yactfr::OptionalType& optType, const Size indent,
                                  const Stylist& stylist, boost::optional<std::string> extra,
                                  const Size extraWidth,
                                  std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    _dtDetailsFromDt(optType.dataType(), indent + 2, stylist, boost::none, 0, std::move(extra),
                     extraWidth, vec);
}

template <typename OptTypeT>
void _dtDetailsFromOptWithIntSelType(const OptTypeT& optType, const Size indent,
                                     const Stylist& stylist,
                                     std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    auto rangesStr = intRangeSetStr(optType.selectorRanges());
    const auto extraWidth = rangesStr.size();

    _dtDetailsFromOptType(optType, indent, stylist, std::move(rangesStr), extraWidth, vec);
}

static void _dtDetailsFromDt(const yactfr::DataType& dt, const Size indent, const Stylist& stylist,
                             const boost::optional<std::string>& name, const Size nameWidth,
                             std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    _dtDetailsFromDt(dt, indent, stylist, name, nameWidth, boost::none, 0, vec);
}

static void _dtDetailsFromDt(const yactfr::DataType& dt, const Size indent, const Stylist& stylist,
                             const boost::optional<std::string>& name, const Size nameWidth,
                             boost::optional<std::string> extra, const Size extraWidth,
                             std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    vec.push_back(std::make_unique<const DtDetails>(dt, name, nameWidth, std::move(extra),
                                                    extraWidth, indent, stylist));

    if (dt.isStructureType()) {
        auto& structType = dt.asStructureType();

        if (structType.isEmpty()) {
            return;
        }

        const auto maxDispNameIt = std::max_element(structType.begin(), structType.end(),
                                                    [](auto& memberTypeA, auto& memberTypeB) {
            return memberTypeA->displayName()->size() < memberTypeB->displayName()->size();
        });

        for (auto& memberType : structType) {
            _dtDetailsFromDt(memberType->dataType(), indent + 2, stylist,
                             *memberType->displayName(), (*maxDispNameIt)->displayName()->size(),
                             vec);
        }
    } else if (dt.isVariantWithUnsignedIntegerSelectorType()) {
        _dtDetailsFromVarType(dt.asVariantWithUnsignedIntegerSelectorType(), indent, stylist, vec);
    } else if (dt.isVariantWithUnsignedIntegerSelectorType()) {
        _dtDetailsFromVarType(dt.asVariantWithSignedIntegerSelectorType(), indent, stylist, vec);
    } else if (dt.isOptionalWithBooleanSelectorType()) {
        _dtDetailsFromOptType(dt.asOptionalWithBooleanSelectorType(), indent, stylist, boost::none,
                              0, vec);
    } else if (dt.isOptionalWithUnsignedIntegerSelectorType()) {
        _dtDetailsFromOptWithIntSelType(dt.asOptionalWithUnsignedIntegerSelectorType(), indent,
                                        stylist, vec);
    } else if (dt.isOptionalWithSignedIntegerSelectorType()) {
        _dtDetailsFromOptWithIntSelType(dt.asOptionalWithSignedIntegerSelectorType(), indent,
                                        stylist, vec);
    } else if (dt.isArrayType()) {
        _dtDetailsFromDt(dt.asArrayType().elementType(), indent + 2, stylist, boost::none, 0, vec);
    } else if (dt.isFixedLengthSignedEnumerationType()) {
        _enumTypeMappingDetailsFromDt(dt.asFixedLengthSignedEnumerationType(), indent + 2, stylist,
                                      vec);
    } else if (dt.isFixedLengthUnsignedEnumerationType()) {
        _enumTypeMappingDetailsFromDt(dt.asFixedLengthUnsignedEnumerationType(), indent + 2,
                                      stylist, vec);
    } else if (dt.isVariableLengthSignedEnumerationType()) {
        _enumTypeMappingDetailsFromDt(dt.asVariableLengthSignedEnumerationType(), indent + 2,
                                      stylist, vec);
    } else if (dt.isVariableLengthUnsignedEnumerationType()) {
        _enumTypeMappingDetailsFromDt(dt.asVariableLengthUnsignedEnumerationType(), indent + 2,
                                      stylist, vec);
    }
}

void dtDetailsFromDt(const yactfr::DataType& dt, const Stylist& stylist,
                     std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    _dtDetailsFromDt(dt, 0, stylist, boost::none, 0, vec);
}

} // namespace jacques
