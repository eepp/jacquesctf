/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <yactfr/metadata/data-type.hpp>
#include <yactfr/metadata/int-type.hpp>
#include <yactfr/metadata/float-type.hpp>
#include <yactfr/metadata/string-type.hpp>
#include <yactfr/metadata/struct-type.hpp>
#include <yactfr/metadata/static-array-type.hpp>
#include <yactfr/metadata/dynamic-array-type.hpp>
#include <yactfr/metadata/variant-type.hpp>
#include <yactfr/metadata/clock-type.hpp>
#include <yactfr/metadata/field-ref.hpp>

#include "dt-details.hpp"

namespace jacques {

DtDetails::DtDetails(const yactfr::DataType& dt, const boost::optional<std::string>& name,
                     const Size nameWidth, const Size indent, const Stylist& stylist) noexcept :
    AbstractDtDetails {indent, stylist},
    _dt {&dt},
    _name {name},
    _nameWidth {nameWidth}
{
}

void DtDetails::_renderLine(WINDOW *const window, const Size maxWidth, const bool stylize) const
{
    auto remWidth = maxWidth;

    this->_renderName(window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    // TODO: use data type visitor
    if (_dt->isIntType()) {
        this->_renderDt(*_dt->asIntType(), window, remWidth, stylize);
    } else if (_dt->isFloatType()) {
        this->_renderDt(*_dt->asFloatType(), window, remWidth, stylize);
    } else if (_dt->isStringType()) {
        this->_renderDt(*_dt->asStringType(), window, remWidth, stylize);
    } else if (_dt->isStructType()) {
        this->_renderDt(*_dt->asStructType(), window, remWidth, stylize);
    } else if (_dt->isStaticArrayType()) {
        this->_renderDt(*_dt->asStaticArrayType(), window, remWidth, stylize);
    } else if (_dt->isDynamicArrayType()) {
        this->_renderDt(*_dt->asDynamicArrayType(), window, remWidth, stylize);
    } else if (_dt->isVariantType()) {
        this->_renderDt(*_dt->asVariantType(), window, remWidth, stylize);
    } else {
        std::abort();
    }
}

static const char *boStr(const yactfr::ByteOrder bo) noexcept
{
    return (bo == yactfr::ByteOrder::BIG) ? "be" : "le";
}

void DtDetails::_renderDt(const yactfr::IntType& dt, WINDOW * const window, const Size maxWidth,
                          const bool stylize) const
{
    std::ostringstream ss;

    ss << '{';

    if (dt.isSignedEnumType()) {
        ss << "ie";
    } else if (dt.isUnsignedEnumType()) {
        ss << "ue";
    } else if (dt.isSignedIntType()) {
        ss << 'i';
    } else if (dt.isUnsignedIntType()) {
        ss << 'u';
    }

    ss << dt.size() << ' ' << boStr(dt.byteOrder()) << " %" << dt.alignment() << '}';

    auto remWidth = maxWidth;

    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    const char *dbase = [&dt] {
        switch (dt.displayBase()) {
        case yactfr::DisplayBase::BINARY:
            return "bin";

        case yactfr::DisplayBase::OCTAL:
            return "oct";

        case yactfr::DisplayBase::DECIMAL:
            return "dec";

        case yactfr::DisplayBase::HEXADECIMAL:
            return "hex";

        default:
            std::abort();
        }
    }();

    assert(dbase);
    this->_renderProp(window, remWidth, stylize, "disp-base", dbase);

    if (remWidth == 0) {
        return;
    }

    if (dt.mappedClockType()) {
        this->_renderProp(window, remWidth, stylize, "mapped-clk-type",
                          dt.mappedClockType()->name().c_str());
    }
}

void DtDetails::_renderDt(const yactfr::FloatType& dt, WINDOW * const window, const Size maxWidth,
                          const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    ss << "{flt" << dt.size() << ' ' << boStr(dt.byteOrder()) << " %" << dt.alignment() << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());
}

void DtDetails::_renderDt(const yactfr::StringType& type, WINDOW * const window,
                          const Size maxWidth, const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    ss << "{string %" << type.alignment() << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());
}

void DtDetails::_renderDt(const yactfr::StructType& dt, WINDOW * const window,
                          const Size maxWidth, const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    ss << "{struct %" << dt.alignment() << '}';

    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "field-cnt",
                      static_cast<unsigned long long>(dt.fields().size()));
}

void DtDetails::_renderDt(const yactfr::StaticArrayType& dt, WINDOW * const window,
                          const Size maxWidth, const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    if (dt.isStaticTextArrayType()) {
        ss << "{s-text-array %";
    } else {
        ss << "{s-array %";
    }

    ss << dt.alignment() << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "len",
                      static_cast<unsigned long long>(dt.length()));
}

void DtDetails::_renderDt(const yactfr::DynamicArrayType& dt, WINDOW * const window,
                          const Size maxWidth, const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    if (dt.isDynamicTextArrayType()) {
        ss << "{d-text-array %";
    } else {
        ss << "{d-array %";
    }

    ss << dt.alignment() << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderFieldRef(window, remWidth, stylize, "len-ref", dt.length());
}

void DtDetails::_renderDt(const yactfr::VariantType& dt, WINDOW * const window,
                          const Size maxWidth, const bool stylize) const
{
    std::ostringstream ss;
    auto remWidth = maxWidth;

    ss << "{var %" << dt.alignment() << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderFieldRef(window, remWidth, stylize, "tag-ref", dt.tag());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "option-cnt",
                      static_cast<unsigned long long>(dt.options().size()));
}

void DtDetails::_renderName(WINDOW * const window, Size& remWidth, const bool stylize) const
{
    if (!_name) {
        return;
    }

    if (stylize) {
        this->_stylist().detailsViewDtName(window);
    }

    auto& name = *_name;

    this->_renderStr(window, remWidth, *_name);

    if (remWidth == 0) {
        return;
    }

    const auto spCount = _nameWidth + 1 - name.size();

    if (spCount >= remWidth) {
        remWidth = 0;
        return;
    }

    if (stylize) {
        this->_stylist().std(window);
    }

    for (Index i = 0; i < spCount; ++i) {
        waddch(window, ' ');
    }

    remWidth -= spCount;
}

void DtDetails::_renderDtInfo(WINDOW * const window, Size& remWidth, const bool stylize,
                              const char * const info) const
{
    if (stylize) {
        this->_stylist().detailsViewTypeInfo(window);
    }

    this->_renderStr(window, remWidth, info);
}

void DtDetails::_renderProp(WINDOW * const window, Size& remWidth, const bool stylize,
                            const char * const key, const char * const val) const
{
    if (stylize) {
        this->_stylist().detailsViewPropKey(window);
    }

    this->_renderChar(window, remWidth, ' ');

    if (remWidth == 0) {
        return;
    }

    this->_renderStr(window, remWidth, key);

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_stylist().std(window);
    }

    this->_renderChar(window, remWidth, '=');

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_stylist().detailsViewPropVal(window);
    }

    this->_renderStr(window, remWidth, val);
}

void DtDetails::_renderProp(WINDOW * const window, Size& remWidth, const bool stylize,
                            const char * const key, const unsigned long long val) const
{
    std::ostringstream ss;

    ss << val;
    this->_renderProp(window, remWidth, stylize, key, ss.str().c_str());
}

void DtDetails::_renderFieldRef(WINDOW * const window, Size& remWidth, const bool stylize,
                                const char * const key, const yactfr::FieldRef& ref) const
{
    std::ostringstream ss;

    switch (ref.scope()) {
    case yactfr::Scope::PACKET_HEADER:
        ss << "PH";
        break;

    case yactfr::Scope::PACKET_CONTEXT:
        ss << "PC";
        break;

    case yactfr::Scope::EVENT_RECORD_HEADER:
        ss << "ERH";
        break;

    case yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT:
        ss << "ER1C";
        break;

    case yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT:
        ss << "ER2C";
        break;

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        ss << "ERP";
        break;
    }

    for (auto& pathElement : ref.pathElements()) {
        ss << '/' << pathElement;
    }

    this->_renderProp(window, remWidth, stylize, key, ss.str().c_str());
}

} // namespace jacques
