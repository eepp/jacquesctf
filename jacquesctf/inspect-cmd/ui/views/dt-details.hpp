/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP

#include <string>
#include <sstream>
#include <optional>
#include <functional>
#include <notcurses/notcurses.h>

#include "abstract-dt-details.hpp"

namespace jacques {

class DtDetails final :
    public AbstractDtDetails
{
public:
    explicit DtDetails(const yactfr::DataType& dt, std::optional<std::string> name,
                       Size nameWidth, std::optional<std::string> extra, Size extraWidth,
                       Size indent, const Stylist& stylist) noexcept;

    explicit DtDetails(const yactfr::DataType& dt, std::optional<std::string> name,
                       Size nameWidth, Size indent, const Stylist& stylist) noexcept;

    const yactfr::DataType& dt() const noexcept
    {
        return *_dt;
    }

    const std::optional<std::string>& name() const noexcept
    {
        return _name;
    }

    const std::optional<std::string>& extra() const noexcept
    {
        return _extra;
    }

private:
    void _renderLine(ncplane& plane, Size maxWidth, bool stylize) const override;

    void _tryRenderPrefDispBaseProp(const yactfr::DataType& dt, ncplane& plane, Size& remWidth,
                                    bool stylize) const;

    void _renderDt(const yactfr::FixedLengthBitArrayType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::NullTerminatedStringType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StaticLengthStringType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthStringType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _tryRenderMediaTypeProp(const yactfr::BlobType& dt, ncplane& plane, Size remWidth,
                                 bool stylize) const;

    void _renderDt(const yactfr::StaticLengthBlobType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthBlobType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StaticLengthArrayType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthArrayType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StructureType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::OptionalType& dt, ncplane& plane, Size remWidth,
                   bool stylize) const;

    template <typename IntTypeT>
    void _renderMappingCountProp(const IntTypeT& dt, ncplane& plane, Size& remWidth,
                                 const bool stylize) const
    {
        if (!dt.mappings().empty()) {
            this->_renderProp(plane, remWidth, stylize, "mapping-cnt", dt.mappings().size());
        }
    }

    template <typename IntTypeT>
    void _renderIntTypeCommon(const IntTypeT& dt, ncplane& plane, Size remWidth,
                              const bool stylize) const
    {
        if (remWidth == 0) {
            return;
        }

        this->_tryRenderPrefDispBaseProp(dt, plane, remWidth, stylize);

        if (remWidth == 0) {
            return;
        }

        if (dt.isFixedLengthUnsignedIntegerType()) {
            this->_renderRoleFlags(dt.asFixedLengthUnsignedIntegerType(), plane, remWidth,
                                   stylize);
        } else if (dt.isVariableLengthUnsignedIntegerType()) {
            this->_renderRoleFlags(dt.asVariableLengthUnsignedIntegerType(), plane, remWidth,
                                   stylize);
        }

        if (remWidth == 0) {
            return;
        }

        this->_renderMappingCountProp(dt, plane, remWidth, stylize);
    }

    template <typename VlIntTypeT>
    void _renderVlIntType(const VlIntTypeT& dt, ncplane& plane, Size remWidth,
                          const bool stylize) const
    {
        std::ostringstream ss;

        ss << '{';

        if (dt.isVariableLengthUnsignedIntegerType()) {
            ss << "~u";
        } else {
            assert(dt.isVariableLengthSignedIntegerType());
            ss << "~i";
        }

        ss << '}';
        this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());
        this->_renderIntTypeCommon(dt, plane, remWidth, stylize);
    }

    template <typename DtT>
    void _tryRenderHasTraceTypeUuidRoleFlag(const DtT& dt, ncplane& plane, Size& remWidth,
                                            const bool stylize) const
    {
        if (dt.hasMetadataStreamUuidRole()) {
            this->_renderRoleFlag(plane, remWidth, stylize, "metadata-stream-uuid");
        }
    }

    template <typename DtT>
    void _tryRenderMinAlignProp(const DtT& dt, ncplane& plane, Size& remWidth, const bool stylize) const
    {
        if (dt.minimumAlignment() == 1) {
            return;
        }

        this->_renderProp(plane, remWidth, stylize, "min-align", dt.minimumAlignment());
    }

    template <typename DtT>
    void _renderRoleFlags(const DtT& dt, ncplane& plane, Size& remWidth, const bool stylize) const
    {
        for (const auto role : dt.roles()) {
            const auto roleStr = std::invoke([role] {
                switch (role) {
                case yactfr::UnsignedIntegerTypeRole::PacketMagicNumber:
                    return "pkt-magic-number";

                case yactfr::UnsignedIntegerTypeRole::DataStreamTypeId:
                    return "dst-id";

                case yactfr::UnsignedIntegerTypeRole::DataStreamId:
                    return "ds-id";

                case yactfr::UnsignedIntegerTypeRole::PacketTotalLength:
                    return "pkt-total-len";

                case yactfr::UnsignedIntegerTypeRole::PacketContentLength:
                    return "pkt-content-len";

                case yactfr::UnsignedIntegerTypeRole::DefaultClockTimestamp:
                    return "def-clk-ts";

                case yactfr::UnsignedIntegerTypeRole::PacketEndDefaultClockTimestamp:
                    return "pkt-end-def-clk-ts";

                case yactfr::UnsignedIntegerTypeRole::DiscardedEventRecordCounterSnapshot:
                    return "disc-er-counter-snap";

                case yactfr::UnsignedIntegerTypeRole::PacketSequenceNumber:
                    return "pkt-seq-num";

                case yactfr::UnsignedIntegerTypeRole::EventRecordTypeId:
                    return "ert-id";

                default:
                    std::abort();
                }
            });

            this->_renderRoleFlag(plane, remWidth, stylize, roleStr);

            if (remWidth == 0) {
                return;
            }
        }
    }

    template <typename DtT>
    void _renderSelLocProp(const DtT& dt, ncplane& plane, Size& remWidth, const bool stylize) const
    {
        this->_renderDataLoc(plane, remWidth, stylize, "sel-loc", dt.selectorLocation());
    }

    static char _varTypeSelCh(const yactfr::VariantWithUnsignedIntegerSelectorType&) noexcept
    {
        return 'u';
    }

    static char _varTypeSelCh(const yactfr::VariantWithSignedIntegerSelectorType&) noexcept
    {
        return 'i';
    }

    template <typename VarTypeT>
    void _renderVarType(const VarTypeT& dt, ncplane& plane, Size remWidth,
                        const bool stylize) const
    {
        std::ostringstream ss;

        ss << "{var-" << this->_varTypeSelCh(dt) << "-sel}";
        this->_renderDtInfo(plane, remWidth, stylize, ss.str().c_str());

        if (remWidth == 0) {
            return;
        }

        this->_renderSelLocProp(dt, plane, remWidth, stylize);

        if (remWidth == 0) {
            return;
        }

        this->_renderProp(plane, remWidth, stylize, "opt-cnt", dt.options().size());
    }

    void _renderName(ncplane& plane, Size& remWidth, bool stylize) const;
    void _renderExtra(ncplane& plane, Size& remWidth, bool stylize) const;
    void _renderDtInfo(ncplane& plane, Size& remWidth, bool stylize, const char *info) const;
    void _renderRoleFlag(ncplane& plane, Size& remWidth, bool stylize, const char *name) const;
    void _renderStrEncodingProp(ncplane& plane, Size& remWidth, bool stylize,
                                const yactfr::StringType& strType) const;

    void _renderProp(ncplane& plane, Size& remWidth, bool stylize, const char *key,
                     const char *val) const;

    void _renderProp(ncplane& plane, Size& remWidth, bool stylize, const char *key,
                     unsigned long long val) const;

    void _renderDataLoc(ncplane& plane, Size& remWidth, bool stylize, const char *key,
                        const yactfr::DataLocation& loc) const;

    template <typename StyliseFuncT, typename UnstyliseFuncT>
    void _renderAligned(ncplane& plane, Size& remWidth, StyliseFuncT&& styliseFunc,
                        UnstyliseFuncT&& unstyliseFunc, const std::optional<std::string>& str,
                        const Size strWidth) const
    {
        if (!str) {
            return;
        }

        styliseFunc(plane);
        this->_renderStr(plane, remWidth, *str);

        if (remWidth == 0) {
            return;
        }

        const auto spCount = strWidth + 1 - dtDetailsStrWidth(*str);

        if (spCount >= remWidth) {
            remWidth = 0;
            return;
        }

        unstyliseFunc(plane);

        for (Index i = 0; i < spCount; ++i) {
            ncplane_putchar(&plane, ' ');
        }

        remWidth -= spCount;
    }

private:
    const yactfr::DataType *_dt;
    const std::optional<std::string> _name;
    const Size _nameWidth;
    const std::optional<std::string> _extra;
    const Size _extraWidth;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
