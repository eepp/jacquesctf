/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_METADATA_HPP
#define _JACQUES_DATA_METADATA_HPP

#include <memory>
#include <unordered_map>
#include <optional>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

#include "data-len.hpp"
#include "dt-path.hpp"

namespace jacques {

class Metadata final
{
public:
    using DtParentMap = std::unordered_map<const yactfr::DataType *, const yactfr::DataType *>;
    using DtScopeMap = std::unordered_map<const yactfr::DataType *, yactfr::Scope>;
    using DtPathMap = std::unordered_map<const yactfr::DataType *, DtPath>;

public:
    explicit Metadata(boost::filesystem::path path, yactfr::TraceType::Up traceType,
                      std::unique_ptr<const yactfr::MetadataStream> stream,
                      std::optional<boost::uuids::uuid> streamUuid);

    Metadata(const Metadata&) = delete;
    Metadata& operator=(const Metadata&) = delete;

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

    const std::optional<boost::uuids::uuid>& streamUuid() const noexcept
    {
        return _streamUuid;
    }

private:
    void _setDtParents();

private:
    const boost::filesystem::path _path;
    yactfr::TraceType::Up _traceType;
    std::unique_ptr<const yactfr::MetadataStream> _stream;
    std::optional<boost::uuids::uuid> _streamUuid;
    DtParentMap _dtParents;
    DtScopeMap _dtScopes;
    DtPathMap _dtPaths;
};

const yactfr::DataLocation *dtDataLoc(const yactfr::DataType& dt) noexcept;

} // namespace jacques

#endif // _JACQUES_DATA_METADATA_HPP
