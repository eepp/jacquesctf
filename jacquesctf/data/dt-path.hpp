/*
 * Copyright (C) 2022 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_DT_PATH_HPP
#define _JACQUES_DATA_DT_PATH_HPP

#include <string>
#include <vector>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <yactfr/yactfr.hpp>

#include "aliases.hpp"

namespace jacques {

class DtPath final
{
public:
    struct StructMemberItem
    {
        Index index;
        std::string name;
    };

    struct VarOptItem
    {
        Index index;
        boost::optional<std::string> name;
    };

    struct CurArrayElemItem
    {
    };

    struct CurOptDataItem
    {
    };

    using Item = boost::variant<StructMemberItem, VarOptItem, CurArrayElemItem, CurOptDataItem>;
    using Items = std::vector<Item>;

public:
    explicit DtPath(const yactfr::Scope scope, Items items);

    yactfr::Scope scope() const noexcept
    {
        return _scope;
    }

    const Items& items() const noexcept
    {
        return _items;
    }

private:
    yactfr::Scope _scope;
    Items _items;
};

} // namespace jacques

#endif // _JACQUES_DATA_DT_PATH_HPP
