/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_TYPE_DETAILS_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_TYPE_DETAILS_HPP

#include <string>
#include <yactfr/metadata/fwd.hpp>
#include <boost/optional.hpp>

#include "abstract-data-type-details.hpp"

namespace jacques {

class DataTypeDetails :
    public AbstractDataTypeDetails
{
public:
    explicit DataTypeDetails(const yactfr::DataType& dataType,
                             const boost::optional<std::string>& name,
                             Size nameWidth, Size indent,
                             const Stylist& stylist);

    const yactfr::DataType& dataType() const noexcept
    {
        return *_dataType;
    }

    const boost::optional<std::string>& name() const noexcept
    {
        return _name;
    }

private:
    void _renderLine(WINDOW *window, Size maxWidth, bool stylize) const override;
    void _renderType(const yactfr::IntType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::FloatType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::StringType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::StructType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::StaticArrayType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::DynamicArrayType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderType(const yactfr::VariantType& type, WINDOW *window,
                     Size maxWidth, bool stylize) const;
    void _renderName(WINDOW *window, Size& remWidth, bool stylize) const;
    void _renderTypeInfo(WINDOW *window, Size& remWidth, bool stylize,
                         const char *info) const;
    void _renderProp(WINDOW *window, Size& remWidth, bool stylize,
                     const char *key, const char *value) const;
    void _renderProp(WINDOW *window, Size& remWidth, bool stylize,
                     const char *key, unsigned long long value) const;
    void _renderFieldRef(WINDOW *window, Size& remWidth, bool stylize,
                         const char *key, const yactfr::FieldRef& ref) const;

private:
    const yactfr::DataType *_dataType;
    const boost::optional<std::string> _name;
    const Size _nameWidth;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_DATA_TYPE_DETAILS_HPP
