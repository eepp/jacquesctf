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

#include "abstract-dt-details.hpp"
#include "dt-details.hpp"
#include "enum-type-member-details.hpp"

namespace jacques {

AbstractDtDetails::AbstractDtDetails(const Size indent,
                                                 const Stylist& stylist) noexcept :
    _indentWidth {indent},
    _theStylist {&stylist}
{
}

void AbstractDtDetails::renderLine(WINDOW * const window, const Size maxWidth,
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

void AbstractDtDetails::_renderChar(WINDOW * const window, Size& remWidth, const char ch) const
{
    if (remWidth > 0) {
        waddch(window, ch);
        --remWidth;
    }
}

void AbstractDtDetails::_renderStr(WINDOW * const window, Size& remWidth, const char * const str) const
{
    assert(remWidth != 0);

    std::array<char, 128> buf;
    const auto maxPrintSize = std::min(remWidth + 1, static_cast<Size>(buf.size()));

    std::snprintf(buf.data(), maxPrintSize, "%s", utils::escapeStr(str).c_str());
    remWidth -= std::strlen(buf.data());
    wprintw(window, "%s", buf.data());
}

template <typename EnumT>
static void _enumTypeMemberDetailsFromDt(const EnumT& dt, const Size indent,
                                         const Stylist& stylist,
                                         std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    using Details = EnumTypeMemberDetails<EnumT>;

    for (auto& nameMemberPair : dt.members()) {
        auto& name = nameMemberPair.first;
        auto& member = nameMemberPair.second;

        vec.push_back(std::make_unique<const Details>(name, member, indent, stylist));
    }
}

static void _dtDetailsFromDt(const yactfr::DataType& dt, const Size indent, const Stylist& stylist,
                             const boost::optional<std::string>& name, const Size nameWidth,
                             std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    vec.push_back(std::make_unique<const DtDetails>(dt, name, nameWidth, indent, stylist));

    if (dt.isStructType()) {
        if (dt.asStructType()->fields().empty()) {
            return;
        }

        const auto maxIt = std::max_element(dt.asStructType()->begin(), dt.asStructType()->end(),
                                            [](auto& fieldA, auto& fieldB) {
            return fieldA->displayName().size() < fieldB->displayName().size();
        });
        const auto nameWidth = (*maxIt)->displayName().size();

        for (auto& field : *dt.asStructType()) {
            _dtDetailsFromDt(field->type(), indent + 2, stylist, field->displayName(), nameWidth,
                             vec);
        }
    } else if (dt.isVariantType()) {
        if (dt.asVariantType()->options().empty()) {
            return;
        }

        const auto maxIt = std::max_element(dt.asVariantType()->begin(), dt.asVariantType()->end(),
                                            [](auto& choiceA, auto& choiceB) {
            return choiceA->displayName().size() < choiceB->displayName().size();
        });
        const auto nameWidth = (*maxIt)->displayName().size();

        for (auto& choice : *dt.asVariantType()) {
            _dtDetailsFromDt(choice->type(), indent + 2, stylist, choice->displayName(), nameWidth,
                             vec);
        }
    } else if (dt.isStaticArrayType()) {
        _dtDetailsFromDt(dt.asStaticArrayType()->elemType(), indent + 2, stylist, boost::none, 0,
                         vec);
    } else if (dt.isDynamicArrayType()) {
        _dtDetailsFromDt(dt.asDynamicArrayType()->elemType(), indent + 2, stylist, boost::none, 0,
                         vec);
    } else if (dt.isSignedEnumType()) {
        _enumTypeMemberDetailsFromDt(*dt.asSignedEnumType(), indent + 2, stylist, vec);
    } else if (dt.isUnsignedEnumType()) {
        _enumTypeMemberDetailsFromDt(*dt.asUnsignedEnumType(), indent + 2, stylist, vec);
    }
}

void dtDetailsFromDt(const yactfr::DataType& dt, const Stylist& stylist,
                     std::vector<std::unique_ptr<const AbstractDtDetails>>& vec)
{
    _dtDetailsFromDt(dt, 0, stylist, boost::none, 0, vec);
}

} // namespace jacques
