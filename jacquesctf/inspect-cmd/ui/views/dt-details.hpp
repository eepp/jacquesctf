/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP

#include <string>
#include <yactfr/metadata/fwd.hpp>
#include <boost/optional.hpp>

#include "abstract-dt-details.hpp"

namespace jacques {

class DtDetails final :
    public AbstractDtDetails
{
public:
    explicit DtDetails(const yactfr::DataType& dt, const boost::optional<std::string>& name,
                       Size nameWidth, Size indent, const Stylist& stylist) noexcept;

    const yactfr::DataType& dt() const noexcept
    {
        return *_dt;
    }

    const boost::optional<std::string>& name() const noexcept
    {
        return _name;
    }

private:
    void _renderLine(WINDOW *window, Size maxWidth, bool stylize) const override;
    void _renderDt(const yactfr::IntType& type, WINDOW *window, Size maxWidth, bool stylize) const;
    void _renderDt(const yactfr::FloatType& type, WINDOW *window, Size maxWidth, bool stylize) const;
    void _renderDt(const yactfr::StringType& type, WINDOW *window, Size maxWidth, bool stylize) const;
    void _renderDt(const yactfr::StructType& type, WINDOW *window, Size maxWidth, bool stylize) const;
    void _renderDt(const yactfr::StaticArrayType& type, WINDOW *window, Size maxWidth,
                   bool stylize) const;
    void _renderDt(const yactfr::DynamicArrayType& type, WINDOW *window, Size maxWidth,
                   bool stylize) const;
    void _renderDt(const yactfr::VariantType& type, WINDOW *window, Size maxWidth, bool stylize) const;
    void _renderName(WINDOW *window, Size& remWidth, bool stylize) const;
    void _renderDtInfo(WINDOW *window, Size& remWidth, bool stylize, const char *info) const;
    void _renderProp(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                     const char *val) const;
    void _renderProp(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                     unsigned long long val) const;
    void _renderFieldRef(WINDOW *window, Size& remWidth, bool stylize, const char *key,
                         const yactfr::FieldRef& ref) const;

private:
    const yactfr::DataType *_dt;
    const boost::optional<std::string> _name;
    const Size _nameWidth;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_DT_DETAILS_HPP
