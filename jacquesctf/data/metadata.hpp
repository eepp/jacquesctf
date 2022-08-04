/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_METADATA_HPP
#define _JACQUES_DATA_METADATA_HPP

#include <memory>
#include <unordered_map>
#include <yactfr/yactfr.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/uuid/uuid.hpp>

#include "aliases.hpp"
#include "bo.hpp"
#include "data-len.hpp"
#include "dt-path.hpp"
#include "metadata-error.hpp"

namespace jacques {

class Metadata final :
    boost::noncopyable
{
public:
    using DtParentMap = std::unordered_map<const yactfr::DataType *, const yactfr::DataType *>;
    using DtScopeMap = std::unordered_map<const yactfr::DataType *, yactfr::Scope>;
    using DtPathMap = std::unordered_map<const yactfr::DataType *, DtPath>;

public:
    explicit Metadata(boost::filesystem::path path, yactfr::TraceType::UP traceType,
                      std::unique_ptr<const yactfr::MetadataStream> stream,
                      boost::optional<boost::uuids::uuid> streamUuid);
    const yactfr::DataType *dtParent(const yactfr::DataType& dt) const noexcept;
    yactfr::Scope dtScope(const yactfr::DataType& dt) const noexcept;
    const DtPath& dtPath(const yactfr::DataType& dt) const noexcept;

    const DtPathMap& dtPaths() const noexcept
    {
        return _dtPaths;
    }

    bool dtIsScopeRoot(const yactfr::DataType& dt) const noexcept;
    DataLen fileLen() const noexcept;

    const std::string& text() const noexcept
    {
        return _stream->text();
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const yactfr::TraceType& traceType() const noexcept
    {
        return *_traceType;
    }

    const yactfr::MetadataStream& stream() const noexcept
    {
        return *_stream;
    }

    const boost::optional<boost::uuids::uuid>& streamUuid() const noexcept
    {
        return _streamUuid;
    }

    // true if all clock types are absolute or have the same UUID
    bool isCorrelatable() const noexcept
    {
        return _isCorrelatable;
    }

private:
    void _setDtParents();
    void _setIsCorrelatable();

private:
    const boost::filesystem::path _path;
    yactfr::TraceType::UP _traceType;
    std::unique_ptr<const yactfr::MetadataStream> _stream;
    boost::optional<boost::uuids::uuid> _streamUuid;
    DtParentMap _dtParents;
    DtScopeMap _dtScopes;
    DtPathMap _dtPaths;
    bool _isCorrelatable = false;
};

const yactfr::DataLocation *dtDataLoc(const yactfr::DataType& dt) noexcept;

} // namespace jacques

#endif // _JACQUES_DATA_METADATA_HPP
