/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <algorithm>
#include <array>
#include <cstring>
#include <yactfr/metadata/data-type-visitor.hpp>
#include <yactfr/metadata/static-array-type.hpp>
#include <yactfr/metadata/dynamic-array-type.hpp>
#include <yactfr/metadata/struct-type.hpp>
#include <yactfr/metadata/variant-type.hpp>

#include "abstract-data-type-details.hpp"
#include "data-type-details.hpp"
#include "enum-data-type-member-details.hpp"

namespace jacques {

AbstractDataTypeDetails::AbstractDataTypeDetails(const Size indent,
                                                 std::shared_ptr<const Stylist> stylist) :
    _indentWidth {indent},
    _myStylist {stylist}
{
}

AbstractDataTypeDetails::~AbstractDataTypeDetails()
{
}

void AbstractDataTypeDetails::renderLine(WINDOW *window, const Size maxWidth,
                                         const bool stylize) const
{
    if (_indentWidth >= maxWidth) {
        return;
    }

    // indent
    for (Index i = 0; i < _indentWidth; ++i) {
        waddch(window, ' ');
    }

    this->_renderLine(window, maxWidth - _indentWidth, stylize);
}

void AbstractDataTypeDetails::_renderChar(WINDOW *window, Size& remWidth,
                                          const char ch) const
{
    if (remWidth > 0) {
        waddch(window, ch);
        --remWidth;
    }
}

void AbstractDataTypeDetails::_renderString(WINDOW *window, Size& remWidth,
                                            const char * const str) const
{
    assert(remWidth != 0);

    std::array<char, 128> buf;
    const auto maxPrintSize = std::min(remWidth + 1,
                                       static_cast<Size>(buf.size()));

    std::snprintf(buf.data(), maxPrintSize, "%s", str);
    remWidth -= std::strlen(buf.data());
    wprintw(window, "%s", buf.data());
}

template <typename EnumT>
static void _enumDataTypeMemberDetailsFromDataType(const EnumT& dataType,
                                                   const Size indent,
                                                   std::shared_ptr<const Stylist> stylist,
                                                   std::vector<std::unique_ptr<const AbstractDataTypeDetails>>& vec)
{
    using Details = EnumDataTypeMemberDetails<EnumT>;

    for (auto& nameMemberPair : dataType.members()) {
        auto& name = nameMemberPair.first;
        auto& member = nameMemberPair.second;

        vec.push_back(std::make_unique<const Details>(name, member, indent,
                                                      stylist));
    }
}

static void _dataTypeDetailsFromDataType(const yactfr::DataType& dataType,
                                         const Size indent,
                                         std::shared_ptr<const Stylist> stylist,
                                         const boost::optional<std::string>& name,
                                         const Size nameWidth,
                                         std::vector<std::unique_ptr<const AbstractDataTypeDetails>>& vec)
{
    vec.push_back(std::make_unique<const DataTypeDetails>(dataType, name,
                                                          nameWidth, indent,
                                                          stylist));

    if (dataType.isStructType()) {
        if (dataType.asStructType()->fields().empty()) {
            return;
        }

        const auto maxIt = std::max_element(dataType.asStructType()->begin(),
                                            dataType.asStructType()->end(),
                                            [](auto& fieldA, auto& fieldB) {
            return fieldA->displayName().size() < fieldB->displayName().size();
        });
        const auto nameWidth = (*maxIt)->displayName().size();

        for (auto& field : *dataType.asStructType()) {
            _dataTypeDetailsFromDataType(field->type(), indent + 2, stylist,
                                         field->displayName(), nameWidth, vec);
        }
    } else if (dataType.isVariantType()) {
        if (dataType.asVariantType()->options().empty()) {
            return;
        }

        const auto maxIt = std::max_element(dataType.asVariantType()->begin(),
                                            dataType.asVariantType()->end(),
                                            [](auto& choiceA, auto& choiceB) {
            return choiceA->displayName().size() < choiceB->displayName().size();
        });
        const auto nameWidth = (*maxIt)->displayName().size();

        for (auto& choice : *dataType.asVariantType()) {
            _dataTypeDetailsFromDataType(choice->type(), indent + 2, stylist,
                                         choice->displayName(), nameWidth, vec);
        }
    } else if (dataType.isStaticArrayType()) {
        _dataTypeDetailsFromDataType(dataType.asStaticArrayType()->elemType(),
                                     indent + 2, stylist, boost::none,
                                     0, vec);
    } else if (dataType.isDynamicArrayType()) {
        _dataTypeDetailsFromDataType(dataType.asDynamicArrayType()->elemType(),
                                     indent + 2, stylist, boost::none,
                                     0, vec);
    } else if (dataType.isSignedEnumType()) {
        _enumDataTypeMemberDetailsFromDataType(*dataType.asSignedEnumType(),
                                               indent + 2, stylist, vec);
    } else if (dataType.isUnsignedEnumType()) {
        _enumDataTypeMemberDetailsFromDataType(*dataType.asUnsignedEnumType(),
                                               indent + 2, stylist, vec);
    }
}

void dataTypeDetailsFromDataType(const yactfr::DataType& dataType,
                                 std::shared_ptr<const Stylist> stylist,
                                 std::vector<std::unique_ptr<const AbstractDataTypeDetails>>& vec)
{
    _dataTypeDetailsFromDataType(dataType, 0, stylist, boost::none, 0, vec);
}

} // namespace jacques
