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

#include "data-type-details.hpp"

namespace jacques {

DataTypeDetails::DataTypeDetails(const yactfr::DataType& dataType,
                                 const boost::optional<std::string>& name,
                                 const Size nameWidth, const Size indent,
                                 std::shared_ptr<const Stylist> stylist) :
    AbstractDataTypeDetails {indent, stylist},
    _dataType {&dataType},
    _name {name},
    _nameWidth {nameWidth}
{
}

void DataTypeDetails::_renderLine(WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    auto remWidth = maxWidth;

    this->_renderName(window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    // TODO: use data type visitor
    if (_dataType->isIntType()) {
        this->_renderType(*_dataType->asIntType(), window, remWidth, stylize);
    } else if (_dataType->isFloatType()) {
        this->_renderType(*_dataType->asFloatType(), window, remWidth, stylize);
    } else if (_dataType->isStringType()) {
        this->_renderType(*_dataType->asStringType(), window, remWidth,
                          stylize);
    } else if (_dataType->isStructType()) {
        this->_renderType(*_dataType->asStructType(), window, remWidth,
                          stylize);
    } else if (_dataType->isStaticArrayType()) {
        this->_renderType(*_dataType->asStaticArrayType(), window, remWidth,
                          stylize);
    } else if (_dataType->isDynamicArrayType()) {
        this->_renderType(*_dataType->asDynamicArrayType(), window, remWidth,
                          stylize);
    } else if (_dataType->isVariantType()) {
        this->_renderType(*_dataType->asVariantType(), window, remWidth,
                          stylize);
    } else {
        std::abort();
    }
}

void DataTypeDetails::_renderType(const yactfr::IntType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    ss << '{';

    if (type.isSignedEnumType()) {
        ss << "ie";
    } else if (type.isUnsignedEnumType()) {
        ss << "ue";
    } else if (type.isSignedIntType()) {
        ss << 'i';
    } else if (type.isUnsignedIntType()) {
        ss << 'u';
    }

    ss << type.size() << ' ';

    if (type.byteOrder() == yactfr::ByteOrder::BIG) {
        ss << "be";
    } else {
        ss << "le";
    }

    ss << ' ' << type.alignment() << '}';

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    const char *dbase = nullptr;

    switch (type.displayBase()) {
    case yactfr::DisplayBase::BINARY:
        dbase = "bin";
        break;

    case yactfr::DisplayBase::OCTAL:
        dbase = "oct";
        break;

    case yactfr::DisplayBase::DECIMAL:
        dbase = "dec";
        break;

    case yactfr::DisplayBase::HEXADECIMAL:
        dbase = "hex";
        break;
    }

    assert(dbase);
    this->_renderProp(window, remWidth, stylize, "disp-base", dbase);

    if (remWidth == 0) {
        return;
    }

    if (type.mappedClockType()) {
        this->_renderProp(window, remWidth, stylize, "mapped-clk-type",
                          type.mappedClockType()->name().c_str());
    }
}

void DataTypeDetails::_renderType(const yactfr::FloatType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    ss << "{flt" << type.size() << ' ';

    if (type.byteOrder() == yactfr::ByteOrder::BIG) {
        ss << "be";
    } else {
        ss << "le";
    }

    ss << ' ' << type.alignment() << '}';

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());
}

void DataTypeDetails::_renderType(const yactfr::StringType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    ss << "{string " << type.alignment() << '}';

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());
}

void DataTypeDetails::_renderType(const yactfr::StructType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    ss << "{struct " << type.alignment() << '}';

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "field-cnt",
                      static_cast<unsigned long long>(type.fields().size()));
}

void DataTypeDetails::_renderType(const yactfr::StaticArrayType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    if (type.isStaticTextArrayType()) {
        ss << "{s-text-array " << type.alignment() << '}';
    } else {
        ss << "{s-array " << type.alignment() << '}';
    }

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "len",
                      static_cast<unsigned long long>(type.length()));
}

void DataTypeDetails::_renderType(const yactfr::DynamicArrayType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    if (type.isDynamicTextArrayType()) {
        ss << "{d-text-array " << type.alignment() << '}';
    } else {
        ss << "{d-array " << type.alignment() << '}';
    }

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderFieldRef(window, remWidth, stylize, "len-ref",
                          type.length());
}

void DataTypeDetails::_renderType(const yactfr::VariantType& type,
                                  WINDOW *window, const Size maxWidth,
                                  const bool stylize) const
{
    std::ostringstream ss;

    ss << "{var " << type.alignment() << '}';

    Size remWidth = maxWidth;

    this->_renderTypeInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderFieldRef(window, remWidth, stylize, "tag-ref",
                          type.tag());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "option-cnt",
                      static_cast<unsigned long long>(type.options().size()));
}

void DataTypeDetails::_renderName(WINDOW *window, Size& remWidth,
                                  const bool stylize) const
{
    if (!_name) {
        return;
    }

    if (stylize) {
        this->_stylist().detailsViewDataTypeName(window);
    }

    const auto& name = *_name;

    this->_renderString(window, remWidth, *_name);

    if (remWidth == 0) {
        return;
    }

    const auto& spCount = _nameWidth + 1 - name.size();

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

void DataTypeDetails::_renderTypeInfo(WINDOW *window, Size& remWidth,
                                      const bool stylize,
                                      const char * const info) const
{
    if (stylize) {
        this->_stylist().detailsViewTypeInfo(window);
    }

    this->_renderString(window, remWidth, info);
}

void DataTypeDetails::_renderProp(WINDOW *window, Size& remWidth,
                                  const bool stylize, const char * const key,
                                  const char * const value) const
{
    if (stylize) {
        this->_stylist().detailsViewPropKey(window);
    }

    this->_renderChar(window, remWidth, ' ');

    if (remWidth == 0) {
        return;
    }

    this->_renderString(window, remWidth, key);

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
        this->_stylist().detailsViewPropValue(window);
    }

    this->_renderString(window, remWidth, value);
}

void DataTypeDetails::_renderProp(WINDOW *window, Size& remWidth,
                                  const bool stylize, const char * const key,
                                  const unsigned long long value) const
{
    std::ostringstream ss;

    ss << value;
    this->_renderProp(window, remWidth, stylize, key, ss.str().c_str());
}

void DataTypeDetails::_renderFieldRef(WINDOW *window, Size& remWidth,
                                      const bool stylize,
                                      const char * const key,
                                      const yactfr::FieldRef& ref) const
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
