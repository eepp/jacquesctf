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
#include <yactfr/yactfr.hpp>

#include "dt-details.hpp"

namespace jacques {

DtDetails::DtDetails(const yactfr::DataType& dt, boost::optional<std::string> name,
                     const Size nameWidth, boost::optional<std::string> extra,
                     const Size extraWidth, const Size indent, const Stylist& stylist) noexcept :
    AbstractDtDetails {indent, stylist},
    _dt {&dt},
    _name {std::move(name)},
    _nameWidth {nameWidth},
    _extra {std::move(extra)},
    _extraWidth {extraWidth}
{
}

DtDetails::DtDetails(const yactfr::DataType& dt, boost::optional<std::string> name,
                     const Size nameWidth, const Size indent, const Stylist& stylist) noexcept :
    DtDetails {dt, std::move(name), nameWidth, boost::none, 0, indent, stylist}
{
}

void DtDetails::_renderLine(WINDOW *const window, const Size maxWidth, const bool stylize) const
{
    auto remWidth = maxWidth;

    this->_renderName(window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_renderExtra(window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    // TODO: use data type visitor
    if (_dt->isFixedLengthBitArrayType()) {
        this->_renderDt(_dt->asFixedLengthBitArrayType(), window, remWidth, stylize);
    } else if (_dt->isVariableLengthUnsignedIntegerType()) {
        this->_renderVlIntType(_dt->asVariableLengthUnsignedIntegerType(), window, remWidth,
                               stylize);
    } else if (_dt->isVariableLengthSignedIntegerType()) {
        this->_renderVlIntType(_dt->asVariableLengthSignedIntegerType(), window, remWidth, stylize);
    } else if (_dt->isNullTerminatedStringType()) {
        this->_renderDt(_dt->asNullTerminatedStringType(), window, remWidth, stylize);
    } else if (_dt->isStructureType()) {
        this->_renderDt(_dt->asStructureType(), window, remWidth, stylize);
    } else if (_dt->isStaticLengthArrayType()) {
        this->_renderDt(_dt->asStaticLengthArrayType(), window, remWidth, stylize);
    } else if (_dt->isDynamicLengthArrayType()) {
        this->_renderDt(_dt->asDynamicLengthArrayType(), window, remWidth, stylize);
    } else if (_dt->isStaticLengthStringType()) {
        this->_renderDt(_dt->asStaticLengthStringType(), window, remWidth, stylize);
    } else if (_dt->isDynamicLengthStringType()) {
        this->_renderDt(_dt->asDynamicLengthStringType(), window, remWidth, stylize);
    } else if (_dt->isStaticLengthBlobType()) {
        this->_renderDt(_dt->asStaticLengthBlobType(), window, remWidth, stylize);
    } else if (_dt->isDynamicLengthBlobType()) {
        this->_renderDt(_dt->asDynamicLengthBlobType(), window, remWidth, stylize);
    } else if (_dt->isVariantWithUnsignedIntegerSelectorType()) {
        this->_renderVarType(_dt->asVariantWithUnsignedIntegerSelectorType(), window, remWidth,
                             stylize);
    } else if (_dt->isVariantWithSignedIntegerSelectorType()) {
        this->_renderVarType(_dt->asVariantWithSignedIntegerSelectorType(), window, remWidth,
                             stylize);
    } else if (_dt->isOptionalType()) {
        this->_renderDt(_dt->asOptionalType(), window, remWidth, stylize);
    } else {
        std::abort();
    }
}

void DtDetails::_tryRenderPrefDispBaseProp(const yactfr::DataType& dt, WINDOW * const window,
                                           Size& remWidth, const bool stylize) const
{
    if (!dt.isIntegerType()) {
        return;
    }

    const auto prefDispBase = utils::intTypePrefDispBase(dt);

    if (prefDispBase == yactfr::DisplayBase::DECIMAL) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "pref-disp-base", [prefDispBase] {
        switch (prefDispBase) {
        case yactfr::DisplayBase::BINARY:
            return "bin";

        case yactfr::DisplayBase::OCTAL:
            return "oct";

        case yactfr::DisplayBase::HEXADECIMAL:
            return "hex";

        default:
            std::abort();
        }
    }());
}

namespace {

std::string alignStr(const yactfr::Size align)
{
    if (align == 1) {
        return "";
    } else {
        std::ostringstream ss;

        ss << " %" << align;
        return ss.str();
    }
}

std::string alignStr(const yactfr::DataType& dt)
{
    return alignStr(dt.alignment());
}

} // namespace

void DtDetails::_renderDt(const yactfr::FixedLengthBitArrayType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << '{';

    if (dt.isFixedLengthBitMapType()) {
        ss << "bm";
    } else if (dt.isFixedLengthSignedIntegerType()) {
        ss << 'i';
    } else if (dt.isFixedLengthUnsignedIntegerType()) {
        ss << 'u';
    } else if (dt.isFixedLengthBooleanType()) {
        ss << "bool";
    } else if (dt.isFixedLengthFloatingPointNumberType()) {
        ss << "flt";
    } else {
        ss << "ba";
    }

    ss << dt.length() << ' ' << (dt.byteOrder() == yactfr::ByteOrder::BIG ? "be" : "le");

    if ((dt.byteOrder() == yactfr::ByteOrder::BIG && dt.bitOrder() == yactfr::BitOrder::FIRST_TO_LAST) ||
            (dt.byteOrder() == yactfr::ByteOrder::LITTLE && dt.bitOrder() == yactfr::BitOrder::LAST_TO_FIRST)) {
        // indicate unnatural bit order
        ss << '^';
    }

    ss << alignStr(dt) << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (dt.isFixedLengthBitMapType()) {
        const auto count = dt.asFixedLengthBitMapType().flags().size();

        if (count > 0) {
            this->_renderProp(window, remWidth, stylize, "flag-cnt", count);
        }
    } else if (dt.isFixedLengthUnsignedIntegerType()) {
        this->_renderIntTypeCommon(dt.asFixedLengthUnsignedIntegerType(), window, remWidth,
                                   stylize);
    } else if (dt.isFixedLengthSignedIntegerType()) {
        this->_renderIntTypeCommon(dt.asFixedLengthSignedIntegerType(), window, remWidth, stylize);
    }
}

void DtDetails::_renderDt(const yactfr::NullTerminatedStringType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(window, remWidth, stylize, "{nt-str}");
    this->_renderStrEncodingProp(window, remWidth, stylize, dt);
}

void DtDetails::_renderDt(const yactfr::StructureType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{struct" << alignStr(dt) << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMinAlignProp(dt, window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "member-type-cnt", dt.size());
}

void DtDetails::_renderDt(const yactfr::StaticLengthArrayType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{sl-array" << alignStr(dt) << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "len", dt.length());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderHasTraceTypeUuidRoleFlag(dt, window, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::StaticLengthStringType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(window, remWidth, stylize, "{sl-str}");

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "max-len", dt.maximumLength());
    this->_renderStrEncodingProp(window, remWidth, stylize, dt);
}

void DtDetails::_tryRenderMediaTypeProp(const yactfr::BlobType& dt, WINDOW * const window,
                                        Size remWidth, const bool stylize) const
{
    if (dt.mediaType() == yactfr::BlobType::defaultMediaType()) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "media-type", dt.mediaType().c_str());
}

void DtDetails::_renderDt(const yactfr::StaticLengthBlobType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(window, remWidth, stylize, "{sl-blob}");

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(window, remWidth, stylize, "len", dt.length());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMediaTypeProp(dt, window, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderHasTraceTypeUuidRoleFlag(dt, window, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::DynamicLengthArrayType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{dl-array" << alignStr(dt) << '}';
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(window, remWidth, stylize, "len-loc", dt.lengthLocation());
}

void DtDetails::_renderDt(const yactfr::DynamicLengthStringType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(window, remWidth, stylize, "{dl-str}");

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(window, remWidth, stylize, "max-len-loc", dt.maximumLengthLocation());
    this->_renderStrEncodingProp(window, remWidth, stylize, dt);
}

void DtDetails::_renderDt(const yactfr::DynamicLengthBlobType& dt, WINDOW * const window,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(window, remWidth, stylize, "{dl-blob}");

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(window, remWidth, stylize, "len-loc", dt.lengthLocation());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMediaTypeProp(dt, window, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::OptionalType& dt, WINDOW * const window, Size remWidth,
                          const bool stylize) const
{
    std::ostringstream ss;

    ss << "{opt-" << [&dt] {
        if (dt.isOptionalWithBooleanSelectorType()) {
            return 'b';
        } else if (dt.isOptionalWithUnsignedIntegerSelectorType()) {
            return 'u';
        } else {
            assert(dt.isOptionalWithSignedIntegerSelectorType());
            return 'i';
        }
    }() << "-sel}";
    this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderSelLocProp(dt, window, remWidth, stylize);
}

void DtDetails::_renderName(WINDOW * const window, Size& remWidth, const bool stylize) const
{
    this->_renderAligned(window, remWidth, [this, stylize](const auto window) {
        if (stylize) {
            this->_stylist().detailsViewDtName(window);
        }
    }, [this, stylize](const auto window) {
        if (stylize) {
            this->_stylist().std(window);
        }
    }, _name, _nameWidth);
}

void DtDetails::_renderExtra(WINDOW * const window, Size& remWidth, const bool stylize) const
{
    this->_renderAligned(window, remWidth, [this, stylize](const auto window) {
        if (stylize) {
            this->_stylist().detailsViewDtExtra(window);
        }
    }, [this, stylize](const auto window) {
        if (stylize) {
            this->_stylist().std(window);
        }
    }, _extra, _extraWidth);
}

void DtDetails::_renderDtInfo(WINDOW * const window, Size& remWidth, const bool stylize,
                              const char * const info) const
{
    if (stylize) {
        this->_stylist().detailsViewTypeInfo(window);
    }

    this->_renderStr(window, remWidth, info);
}

void DtDetails::_renderRoleFlag(WINDOW * const window, Size& remWidth, const bool stylize,
                                const char * const name) const
{
    if (stylize) {
        this->_stylist().detailsViewPropKey(window);
    }

    this->_renderChar(window, remWidth, ' ');

    if (remWidth == 0) {
        return;
    }

    std::ostringstream ss;

    ss << '<' << name << '>';
    this->_renderStr(window, remWidth, ss.str());
}

void DtDetails::_renderStrEncodingProp(WINDOW * const window, Size& remWidth, const bool stylize,
                                       const yactfr::StringType& strType) const
{
    if (strType.encoding() == yactfr::StringEncoding::UTF_8) {
        return;
    }

    if (remWidth == 0) {
        return;
    }

    std::string enc;

    switch (strType.encoding()) {
    case yactfr::StringEncoding::UTF_16LE:
        enc = "u16le";
        break;

    case yactfr::StringEncoding::UTF_16BE:
        enc = "u16be";
        break;

    case yactfr::StringEncoding::UTF_32LE:
        enc = "u32le";
        break;

    case yactfr::StringEncoding::UTF_32BE:
        enc = "u32be";
        break;

    default:
        break;
    }

    this->_renderProp(window, remWidth, stylize, "enc", enc.c_str());
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

void DtDetails::_renderDataLoc(WINDOW * const window, Size& remWidth, const bool stylize,
                               const char * const key, const yactfr::DataLocation& loc) const
{
    std::ostringstream ss;

    switch (loc.scope()) {
    case yactfr::Scope::PACKET_HEADER:
        ss << "PH";
        break;

    case yactfr::Scope::PACKET_CONTEXT:
        ss << "PC";
        break;

    case yactfr::Scope::EVENT_RECORD_HEADER:
        ss << "ERH";
        break;

    case yactfr::Scope::EVENT_RECORD_COMMON_CONTEXT:
        ss << "ERCC";
        break;

    case yactfr::Scope::EVENT_RECORD_SPECIFIC_CONTEXT:
        ss << "ERSC";
        break;

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        ss << "ERP";
        break;
    }

    for (auto& pathElem : loc.pathElements()) {
        ss << '/' << pathElem;
    }

    this->_renderProp(window, remWidth, stylize, key, ss.str().c_str());
}

} // namespace jacques
