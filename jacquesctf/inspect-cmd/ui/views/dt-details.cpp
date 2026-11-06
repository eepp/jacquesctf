/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <functional>

#include "dt-details.hpp"
#include "utils.hpp"

namespace jacques {

DtDetails::DtDetails(const yactfr::DataType& dt, std::optional<std::string> name,
                     const Size nameWidth, std::optional<std::string> extra,
                     const Size extraWidth, const Size indent, const Stylist& stylist) noexcept :
    AbstractDtDetails {indent, stylist},
    _dt {&dt},
    _name {std::move(name)},
    _nameWidth {nameWidth},
    _extra {std::move(extra)},
    _extraWidth {extraWidth}
{
}

DtDetails::DtDetails(const yactfr::DataType& dt, std::optional<std::string> name,
                     const Size nameWidth, const Size indent, const Stylist& stylist) noexcept :
    DtDetails {dt, std::move(name), nameWidth, std::nullopt, 0, indent, stylist}
{
}

void DtDetails::_renderLine(ncplane& plane, const Size maxWidth, const bool stylize) const
{
    auto remWidth = maxWidth;

    this->_renderName(plane, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_renderExtra(plane, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    // TODO: use data type visitor
    if (_dt->isFixedLengthBitArrayType()) {
        this->_renderDt(_dt->asFixedLengthBitArrayType(), plane, remWidth, stylize);
    } else if (_dt->isVariableLengthUnsignedIntegerType()) {
        this->_renderVlIntType(_dt->asVariableLengthUnsignedIntegerType(), plane, remWidth,
                               stylize);
    } else if (_dt->isVariableLengthSignedIntegerType()) {
        this->_renderVlIntType(_dt->asVariableLengthSignedIntegerType(), plane, remWidth, stylize);
    } else if (_dt->isNullTerminatedStringType()) {
        this->_renderDt(_dt->asNullTerminatedStringType(), plane, remWidth, stylize);
    } else if (_dt->isStructureType()) {
        this->_renderDt(_dt->asStructureType(), plane, remWidth, stylize);
    } else if (_dt->isStaticLengthArrayType()) {
        this->_renderDt(_dt->asStaticLengthArrayType(), plane, remWidth, stylize);
    } else if (_dt->isDynamicLengthArrayType()) {
        this->_renderDt(_dt->asDynamicLengthArrayType(), plane, remWidth, stylize);
    } else if (_dt->isStaticLengthStringType()) {
        this->_renderDt(_dt->asStaticLengthStringType(), plane, remWidth, stylize);
    } else if (_dt->isDynamicLengthStringType()) {
        this->_renderDt(_dt->asDynamicLengthStringType(), plane, remWidth, stylize);
    } else if (_dt->isStaticLengthBlobType()) {
        this->_renderDt(_dt->asStaticLengthBlobType(), plane, remWidth, stylize);
    } else if (_dt->isDynamicLengthBlobType()) {
        this->_renderDt(_dt->asDynamicLengthBlobType(), plane, remWidth, stylize);
    } else if (_dt->isVariantWithUnsignedIntegerSelectorType()) {
        this->_renderVarType(_dt->asVariantWithUnsignedIntegerSelectorType(), plane, remWidth,
                             stylize);
    } else if (_dt->isVariantWithSignedIntegerSelectorType()) {
        this->_renderVarType(_dt->asVariantWithSignedIntegerSelectorType(), plane, remWidth,
                             stylize);
    } else if (_dt->isOptionalType()) {
        this->_renderDt(_dt->asOptionalType(), plane, remWidth, stylize);
    } else {
        std::abort();
    }
}

void DtDetails::_tryRenderPrefDispBaseProp(const yactfr::DataType& dt, ncplane& plane,
                                           Size& remWidth, const bool stylize) const
{
    if (!dt.isIntegerType()) {
        return;
    }

    const auto prefDispBase = utils::intTypePrefDispBase(dt);

    if (prefDispBase == yactfr::DisplayBase::Decimal) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "pref-disp-base", std::invoke([prefDispBase] {
        switch (prefDispBase) {
        case yactfr::DisplayBase::Binary:
            return "bin";

        case yactfr::DisplayBase::Octal:
            return "oct";

        case yactfr::DisplayBase::Hexadecimal:
            return "hex";

        default:
            std::abort();
        }
    }));
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

void DtDetails::_renderDt(const yactfr::FixedLengthBitArrayType& dt, ncplane& plane,
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

    ss << dt.length() << ' ' << (dt.byteOrder() == yactfr::ByteOrder::Big ? "be" : "le");

    if ((dt.byteOrder() == yactfr::ByteOrder::Big && dt.bitOrder() == yactfr::BitOrder::FirstToLast) ||
            (dt.byteOrder() == yactfr::ByteOrder::Little && dt.bitOrder() == yactfr::BitOrder::LastToFirst)) {
        // indicate unnatural bit order
        ss << '^';
    }

    ss << alignStr(dt) << '}';
    this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

    if (dt.isFixedLengthBitMapType()) {
        const auto count = dt.asFixedLengthBitMapType().flags().size();

        if (count > 0) {
            this->_renderProp(plane, remWidth, stylize, "flag-cnt", count);
        }
    } else if (dt.isFixedLengthUnsignedIntegerType()) {
        this->_renderIntTypeCommon(dt.asFixedLengthUnsignedIntegerType(), plane, remWidth,
                                   stylize);
    } else if (dt.isFixedLengthSignedIntegerType()) {
        this->_renderIntTypeCommon(dt.asFixedLengthSignedIntegerType(), plane, remWidth, stylize);
    }
}

void DtDetails::_renderDt(const yactfr::NullTerminatedStringType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(plane, remWidth, stylize, "{nt-str}");
    this->_renderStrEncodingProp(plane, remWidth, stylize, dt);
}

void DtDetails::_renderDt(const yactfr::StructureType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{struct" << alignStr(dt) << '}';
    this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMinAlignProp(dt, plane, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "member-type-cnt", dt.size());
}

void DtDetails::_renderDt(const yactfr::StaticLengthArrayType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{sl-array" << alignStr(dt) << '}';
    this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "len", dt.length());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderHasTraceTypeUuidRoleFlag(dt, plane, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::StaticLengthStringType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(plane, remWidth, stylize, "{sl-str}");

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "max-len", dt.maximumLength());
    this->_renderStrEncodingProp(plane, remWidth, stylize, dt);
}

void DtDetails::_tryRenderMediaTypeProp(const yactfr::BlobType& dt, ncplane& plane,
                                        Size remWidth, const bool stylize) const
{
    if (dt.mediaType() == yactfr::BlobType::defaultMediaType()) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "media-type", dt.mediaType().c_str());
}

void DtDetails::_renderDt(const yactfr::StaticLengthBlobType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(plane, remWidth, stylize, "{sl-blob}");

    if (remWidth == 0) {
        return;
    }

    this->_renderProp(plane, remWidth, stylize, "len", dt.length());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMediaTypeProp(dt, plane, remWidth, stylize);

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderHasTraceTypeUuidRoleFlag(dt, plane, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::DynamicLengthArrayType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    std::ostringstream ss;

    ss << "{dl-array" << alignStr(dt) << '}';
    this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(plane, remWidth, stylize, "len-loc", dt.lengthLocation());
}

void DtDetails::_renderDt(const yactfr::DynamicLengthStringType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(plane, remWidth, stylize, "{dl-str}");

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(plane, remWidth, stylize, "max-len-loc", dt.maximumLengthLocation());
    this->_renderStrEncodingProp(plane, remWidth, stylize, dt);
}

void DtDetails::_renderDt(const yactfr::DynamicLengthBlobType& dt, ncplane& plane,
                          Size remWidth, const bool stylize) const
{
    this->_renderDtInfo(plane, remWidth, stylize, "{dl-blob}");

    if (remWidth == 0) {
        return;
    }

    this->_renderDataLoc(plane, remWidth, stylize, "len-loc", dt.lengthLocation());

    if (remWidth == 0) {
        return;
    }

    this->_tryRenderMediaTypeProp(dt, plane, remWidth, stylize);
}

void DtDetails::_renderDt(const yactfr::OptionalType& dt, ncplane& plane, Size remWidth,
                          const bool stylize) const
{
    std::ostringstream ss;

    ss << "{opt-" << std::invoke([&dt] {
        if (dt.isOptionalWithBooleanSelectorType()) {
            return 'b';
        } else if (dt.isOptionalWithUnsignedIntegerSelectorType()) {
            return 'u';
        } else {
            assert(dt.isOptionalWithSignedIntegerSelectorType());
            return 'i';
        }
    }) << "-sel}";
    this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

    if (remWidth == 0) {
        return;
    }

    this->_renderSelLocProp(dt, plane, remWidth, stylize);
}

void DtDetails::_renderName(ncplane& plane, Size& remWidth, const bool stylize) const
{
    this->_renderAligned(plane, remWidth, [this, stylize](auto& plane) {
        if (stylize) {
            this->_style(plane, Stylist::Role::DT_NAME);
        }
    }, [this, stylize](auto& plane) {
        if (stylize) {
            this->_style(plane, Stylist::Role::STD);
        }
    }, _name, _nameWidth);
}

void DtDetails::_renderExtra(ncplane& plane, Size& remWidth, const bool stylize) const
{
    this->_renderAligned(plane, remWidth, [this, stylize](auto& plane) {
        if (stylize) {
            this->_style(plane, Stylist::Role::DT_EXTRA);
        }
    }, [this, stylize](auto& plane) {
        if (stylize) {
            this->_style(plane, Stylist::Role::STD);
        }
    }, _extra, _extraWidth);
}

void DtDetails::_renderDtInfo(ncplane& plane, Size& remWidth, const bool stylize,
                              const char * const info) const
{
    if (stylize) {
        this->_style(plane, Stylist::Role::SYMBOL);
    }

    this->_renderStr(plane, remWidth, info);
}

void DtDetails::_renderRoleFlag(ncplane& plane, Size& remWidth, const bool stylize,
                                const char * const name) const
{
    if (stylize) {
        this->_style(plane, Stylist::Role::PROP_KEY);
    }

    this->_renderChar(plane, remWidth, ' ');

    if (remWidth == 0) {
        return;
    }

    std::ostringstream ss;

    ss << '<' << name << '>';
    this->_renderStr(plane, remWidth, ss.str());
}

void DtDetails::_renderStrEncodingProp(ncplane& plane, Size& remWidth, const bool stylize,
                                       const yactfr::StringType& strType) const
{
    if (strType.encoding() == yactfr::StringEncoding::Utf8) {
        return;
    }

    if (remWidth == 0) {
        return;
    }

    std::string enc;

    switch (strType.encoding()) {
    case yactfr::StringEncoding::Utf16Le:
        enc = "u16le";
        break;

    case yactfr::StringEncoding::Utf16Be:
        enc = "u16be";
        break;

    case yactfr::StringEncoding::Utf32Le:
        enc = "u32le";
        break;

    case yactfr::StringEncoding::Utf32Be:
        enc = "u32be";
        break;

    default:
        break;
    }

    this->_renderProp(plane, remWidth, stylize, "enc", enc.c_str());
}

void DtDetails::_renderProp(ncplane& plane, Size& remWidth, const bool stylize,
                            const char * const key, const char * const val) const
{
    if (stylize) {
        this->_style(plane, Stylist::Role::PROP_KEY);
    }

    this->_renderChar(plane, remWidth, ' ');

    if (remWidth == 0) {
        return;
    }

    this->_renderStr(plane, remWidth, key);

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_style(plane, Stylist::Role::STD);
    }

    this->_renderChar(plane, remWidth, '=');

    if (remWidth == 0) {
        return;
    }

    if (stylize) {
        this->_style(plane, Stylist::Role::PROP_VAL);
    }

    this->_renderStr(plane, remWidth, val);
}

void DtDetails::_renderProp(ncplane& plane, Size& remWidth, const bool stylize,
                            const char * const key, const unsigned long long val) const
{
    std::ostringstream ss;

    ss << val;
    this->_renderProp(plane, remWidth, stylize, key, ss.str().c_str());
}

void DtDetails::_renderDataLoc(ncplane& plane, Size& remWidth, const bool stylize,
                               const char * const key, const yactfr::DataLocation& loc) const
{
    std::ostringstream ss;

    switch (loc.scope()) {
    case yactfr::Scope::PacketHeader:
        ss << "PH";
        break;

    case yactfr::Scope::PacketContext:
        ss << "PC";
        break;

    case yactfr::Scope::EventRecordHeader:
        ss << "ERH";
        break;

    case yactfr::Scope::EventRecordCommonContext:
        ss << "ERCC";
        break;

    case yactfr::Scope::EventRecordSpecificContext:
        ss << "ERSC";
        break;

    case yactfr::Scope::EventRecordPayload:
        ss << "ERP";
        break;
    }

    for (auto& pathElem : loc.pathElements()) {
        ss << '/' << pathElem;
    }

    this->_renderProp(plane, remWidth, stylize, key, ss.str().c_str());
}

} // namespace jacques
