/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP

#include <string>
#include <yactfr/yactfr.hpp>
#include <boost/optional.hpp>

#include "abstract-dt-details.hpp"

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

    void _renderDt(const yactfr::VariableLengthBitArrayType& dt, WINDOW *window, Size remWidth,
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

    template <typename DtT>
    void _tryRenderHasTraceTypeUuidRoleFlag(const DtT& dt, WINDOW * const window, Size& remWidth,
                                            const bool stylize) const
    {
        if (dt.hasTraceTypeUuidRole()) {
            this->_renderRoleFlag(window, remWidth, stylize, "trace-type-uuid");
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
            const auto roleStr = [role] {
                switch (role) {
                case yactfr::UnsignedIntegerTypeRole::PACKET_MAGIC_NUMBER:
                    return "pkt-magic-number";

                case yactfr::UnsignedIntegerTypeRole::DATA_STREAM_TYPE_ID:
                    return "dst-id";

                case yactfr::UnsignedIntegerTypeRole::DATA_STREAM_ID:
                    return "ds-id";

                case yactfr::UnsignedIntegerTypeRole::PACKET_TOTAL_LENGTH:
                    return "pkt-total-len";

                case yactfr::UnsignedIntegerTypeRole::PACKET_CONTENT_LENGTH:
                    return "pkt-content-len";

                case yactfr::UnsignedIntegerTypeRole::DEFAULT_CLOCK_TIMESTAMP:
                    return "def-clk-ts";

                case yactfr::UnsignedIntegerTypeRole::PACKET_END_DEFAULT_CLOCK_TIMESTAMP:
                    return "pkt-end-def-clk-ts";

                case yactfr::UnsignedIntegerTypeRole::DISCARDED_EVENT_RECORD_COUNTER_SNAPSHOT:
                    return "disc-er-counter-snap";

                case yactfr::UnsignedIntegerTypeRole::PACKET_SEQUENCE_NUMBER:
                    return "pkt-seq-num";

                case yactfr::UnsignedIntegerTypeRole::EVENT_RECORD_TYPE_ID:
                    return "ert-id";

                default:
                    std::abort();
                }
            }();

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
