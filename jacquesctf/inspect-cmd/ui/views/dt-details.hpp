/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP

#include <string>
#include <sstream>
#include <yactfr/yactfr.hpp>
#include <boost/optional.hpp>

#include "abstract-dt-details.hpp"
#include "utils.hpp"

namespace jacques {

class DtDetails :
    public AbstractDtDetails
{
public:
    explicit DtDetails(const yactfr::DataType& dt, boost::optional<std::string> name,
                       Size nameWidth, boost::optional<std::string> extra, Size extraWidth,
                       Size indent, const Stylist& stylist) noexcept;

    explicit DtDetails(const yactfr::DataType& dt, boost::optional<std::string> name,
                       Size nameWidth, Size indent, const Stylist& stylist) noexcept;

    const yactfr::DataType& dt() const noexcept
    {
        return *_dt;
    }

    const boost::optional<std::string>& name() const noexcept
    {
        return _name;
    }

    const boost::optional<std::string>& extra() const noexcept
    {
        return _extra;
    }

private:
    void _renderLine(WINDOW *window, Size maxWidth, bool stylize) const override;

    void _tryRenderPrefDispBaseProp(const yactfr::DataType& dt, WINDOW *window, Size& remWidth,
                                    bool stylize) const;

    void _renderDt(const yactfr::FixedLengthBitArrayType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::NullTerminatedStringType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StaticLengthStringType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthStringType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _tryRenderMediaTypeProp(const yactfr::BlobType& dt, WINDOW *window, Size remWidth,
                                 bool stylize) const;

    void _renderDt(const yactfr::StaticLengthBlobType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthBlobType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StaticLengthArrayType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::DynamicLengthArrayType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::StructureType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    void _renderDt(const yactfr::OptionalType& dt, WINDOW *window, Size remWidth,
                   bool stylize) const;

    template <typename IntTypeT>
    void _renderMappingCountProp(const IntTypeT& dt, WINDOW * const window, Size& remWidth,
                                 const bool stylize) const
    {
        if (!dt.mappings().empty()) {
            this->_renderProp(window, remWidth, stylize, "mapping-cnt", dt.mappings().size());
        }
    }

    template <typename IntTypeT>
    void _renderIntTypeCommon(const IntTypeT& dt, WINDOW * const window, Size remWidth,
                              const bool stylize) const
    {
        if (remWidth == 0) {
            return;
        }

        this->_tryRenderPrefDispBaseProp(dt, window, remWidth, stylize);

        if (remWidth == 0) {
            return;
        }

        if (dt.isFixedLengthUnsignedIntegerType()) {
            this->_renderRoleFlags(dt.asFixedLengthUnsignedIntegerType(), window, remWidth,
                                   stylize);
        } else if (dt.isVariableLengthUnsignedIntegerType()) {
            this->_renderRoleFlags(dt.asVariableLengthUnsignedIntegerType(), window, remWidth,
                                   stylize);
        }

        if (remWidth == 0) {
            return;
        }

        this->_renderMappingCountProp(dt, window, remWidth, stylize);
    }

    template <typename VlIntTypeT>
    void _renderVlIntType(const VlIntTypeT& dt, WINDOW * const window, Size remWidth,
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
        this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());
        this->_renderIntTypeCommon(dt, window, remWidth, stylize);
    }

    template <typename DtT>
    void _tryRenderHasTraceTypeUuidRoleFlag(const DtT& dt, WINDOW * const window, Size& remWidth,
                                            const bool stylize) const
    {
        if (dt.hasMetadataStreamUuidRole()) {
            this->_renderRoleFlag(window, remWidth, stylize, "metadata-stream-uuid");
        }
    }

    template <typename DtT>
    void _tryRenderMinAlignProp(const DtT& dt, WINDOW * const window, Size& remWidth, const bool stylize) const
    {
        if (dt.minimumAlignment() == 1) {
            return;
        }

        this->_renderProp(window, remWidth, stylize, "min-align", dt.minimumAlignment());
    }

    template <typename DtT>
    void _renderRoleFlags(const DtT& dt, WINDOW * const window, Size& remWidth, const bool stylize) const
    {
        for (const auto role : dt.roles()) {
            const auto roleStr = utils::call([role] {
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

            this->_renderRoleFlag(window, remWidth, stylize, roleStr);

            if (remWidth == 0) {
                return;
            }
        }
    }

    template <typename DtT>
    void _renderSelLocProp(const DtT& dt, WINDOW * const window, Size& remWidth, const bool stylize) const
    {
        this->_renderDataLoc(window, remWidth, stylize, "sel-loc", dt.selectorLocation());
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
    void _renderVarType(const VarTypeT& dt, WINDOW * const window, Size remWidth,
                        const bool stylize) const
    {
        std::ostringstream ss;

        ss << "{var-" << this->_varTypeSelCh(dt) << "-sel}";
        this->_renderDtInfo(window, remWidth, stylize, ss.str().c_str());

        if (remWidth == 0) {
            return;
        }

        this->_renderSelLocProp(dt, window, remWidth, stylize);

        if (remWidth == 0) {
            return;
        }

        this->_renderProp(window, remWidth, stylize, "opt-cnt", dt.options().size());
    }

    void _renderName(WINDOW *window, Size& remWidth, bool stylize) const;
    void _renderExtra(WINDOW *window, Size& remWidth, bool stylize) const;
    void _renderDtInfo(WINDOW *window, Size& remWidth, bool stylize, const char *info) const;
    void _renderRoleFlag(WINDOW *window, Size& remWidth, bool stylize, const char *name) const;
    void _renderStrEncodingProp(WINDOW *window, Size& remWidth, bool stylize,
                                const yactfr::StringType& strType) const;

    void _renderProp(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                     const char *val) const;

    void _renderProp(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                     unsigned long long val) const;

    void _renderDataLoc(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                        const yactfr::DataLocation& loc) const;

    template <typename StyliseFuncT, typename UnstyliseFuncT>
    void _renderAligned(WINDOW * const window, Size& remWidth, StyliseFuncT&& styliseFunc,
                        UnstyliseFuncT&& unstyliseFunc, const boost::optional<std::string>& str,
                        const Size strWidth) const
    {
        if (!str) {
            return;
        }

        styliseFunc(window);
        this->_renderStr(window, remWidth, *str);

        if (remWidth == 0) {
            return;
        }

        const auto spCount = strWidth + 1 - str->size();

        if (spCount >= remWidth) {
            remWidth = 0;
            return;
        }

        unstyliseFunc(window);

        for (Index i = 0; i < spCount; ++i) {
            waddch(window, ' ');
        }

        remWidth -= spCount;
    }

private:
    const yactfr::DataType *_dt;
    const boost::optional<std::string> _name;
    const Size _nameWidth;
    const boost::optional<std::string> _extra;
    const Size _extraWidth;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
