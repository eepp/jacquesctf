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
#include <yactfr/metadata/metadata-stream.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/metadata/trace-type.hpp>
#include <yactfr/metadata/invalid-metadata.hpp>
#include <yactfr/metadata/invalid-metadata-stream.hpp>
#include <yactfr/metadata/metadata-parse-error.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/core/noncopyable.hpp>

#include "aliases.hpp"
#include "data-len.hpp"
#include "metadata-error.hpp"

namespace jacques {

class Metadata final :
    boost::noncopyable
{
public:
    struct DtPath
    {
        yactfr::Scope scope;
        std::vector<std::string> path;
    };

    using DtParentMap = std::unordered_map<const yactfr::DataType *, const yactfr::DataType *>;

    using DtScopeMap = std::unordered_map<const yactfr::DataType *, yactfr::Scope>;
    using DtPathMap = std::unordered_map<const yactfr::DataType *, DtPath>;

public:
    explicit Metadata(boost::filesystem::path path);
    const yactfr::DataType *dtParent(const yactfr::DataType& dt) const noexcept;
    yactfr::Scope dtScope(const yactfr::DataType& dt) const noexcept;
    const DtPath& dtPath(const yactfr::DataType& dt) const noexcept;

    Size maxDtPathSize() const noexcept
    {
        return _maxDtPathSize;
    }

    bool dtIsScopeRoot(const yactfr::DataType& dt) const noexcept;
    DataLen fileLen() const noexcept;

    const std::string& text() const noexcept
    {
        return _text;
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const boost::optional<Size>& streamPktCount() const noexcept
    {
        return _stream.pktCount;
    }

    const boost::optional<unsigned int>& streamMajorVersion() const noexcept
    {
        return _stream.majorVersion;
    }

    const boost::optional<unsigned int>& streamMinorVersion() const noexcept
    {
        return _stream.minorVersion;
    }

    const boost::optional<yactfr::ByteOrder>& streamBo() const noexcept
    {
        return _stream.bo;
    }

    const boost::optional<boost::uuids::uuid>& streamUuid() const noexcept
    {
        return _stream.uuid;
    }

    yactfr::TraceType::SP traceType() const noexcept
    {
        return _traceType;
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
    std::string _text;

    struct {
        boost::optional<Size> pktCount;
        boost::optional<unsigned int> majorVersion;
        boost::optional<unsigned int> minorVersion;
        boost::optional<yactfr::ByteOrder> bo;
        boost::optional<boost::uuids::uuid> uuid;
    } _stream;

    yactfr::TraceType::SP _traceType;
    DtParentMap _dtParents;
    DtScopeMap _dtScopes;
    DtPathMap _dtPaths;
    Size _maxDtPathSize = 0;
    bool _isCorrelatable = false;
};

} // namespace jacques

#endif // _JACQUES_DATA_METADATA_HPP
