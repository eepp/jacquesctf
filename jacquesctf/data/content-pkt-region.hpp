/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_CONTENT_PKT_REGION_HPP
#define _JACQUES_DATA_CONTENT_PKT_REGION_HPP

#include <set>
#include <string>
#include <optional>
#include <variant>

#include "dt-path.hpp"
#include "pkt-region.hpp"
#include "scope.hpp"

namespace jacques {

class ContentPktRegion final :
    public PktRegion
{
public:
    using Val = std::variant<std::nullptr_t, bool, unsigned long long, long long, double, std::string>;
    using ArrayIndexes = std::vector<Index>;

public:
    /*
     * `arrayIndexes` contains the indexes, in data path item order,
     * of the arrays to reach this datum. In other words, the number
     * of current array element items in `dtPath` matches
     * `arrayIndexes.size()`.
     */
    explicit ContentPktRegion(const PktSegment& segment, Scope::SP scope,
                              const yactfr::DataType& dt, const DtPath& dtPath,
                              ArrayIndexes arrayIndexes, std::optional<Val> val) noexcept;

    const yactfr::DataType& dt() const noexcept
    {
        return *_dt;
    }

    const DtPath& dtPath() const noexcept
    {
        return *_dtPath;
    }

    const ArrayIndexes& arrayIndexes() const noexcept
    {
        return _arrayIndexes;
    }

    const std::optional<Val>& val() const noexcept
    {
        return _val;
    }

    /*
     * Returns the sorted active flag or mapping names of this packet
     * region.
     *
     * val() must have a value.
     *
     * If dt() doesn't return a bit map type or an integer type, then
     * this method returns an empty set.
     */
    std::set<std::string> flagOrMappingNames() const;

private:
    void _accept(PktRegionVisitor& visitor) override;

    template <typename IntTypeT>
    std::unordered_set<const std::string *> _mappingNamesOfVal(const IntTypeT& intType) const
    {
        std::unordered_set<const std::string *> names;

        intType.mappingNamesForValue(std::get<typename IntTypeT::MappingValue>(*_val), names);
        return names;
    }

private:
    const yactfr::DataType *_dt;
    const DtPath *_dtPath;
    ArrayIndexes _arrayIndexes;
    std::optional<Val> _val;
};

} // namespace jacques

#endif // _JACQUES_DATA_CONTENT_PKT_REGION_HPP
