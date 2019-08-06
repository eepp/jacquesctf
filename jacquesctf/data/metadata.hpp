/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_METADATA_HPP
#define _JACQUES_METADATA_HPP

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
#include "data-size.hpp"

namespace jacques {

template <typename SubErrorT>
class MetadataError final :
    public std::runtime_error
{
public:
    explicit MetadataError(const boost::filesystem::path& path,
                           const SubErrorT& subError) :
        std::runtime_error {subError.what()},
        _path {path},
        _subError {subError}
    {
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const SubErrorT& subError() const noexcept
    {
        return _subError;
    }

private:
    const boost::filesystem::path _path;
    const SubErrorT _subError;
};

class Metadata :
    boost::noncopyable
{
public:
    struct DataTypePath
    {
        yactfr::Scope scope;
        std::vector<std::string> path;
    };

    using DataTypeParentMap = std::unordered_map<const yactfr::DataType *,
                                                 const yactfr::DataType *>;
    using DataTypeScopeMap = std::unordered_map<const yactfr::DataType *,
                                                yactfr::Scope>;
    using DataTypePathMap = std::unordered_map<const yactfr::DataType *,
                                               DataTypePath>;

public:
    explicit Metadata(const boost::filesystem::path& path);
    const yactfr::DataType *dataTypeParent(const yactfr::DataType& dataType) const;
    yactfr::Scope dataTypeScope(const yactfr::DataType& dataType) const;
    const DataTypePath& dataTypePath(const yactfr::DataType& dataType) const;

    Size maxDataTypePathSize() const noexcept
    {
        return _maxDataTypePathSize;
    }

    bool dataTypeIsScopeRoot(const yactfr::DataType& dataType) const;
    DataSize fileSize() const noexcept;

    const std::string& text() const noexcept
    {
        return _text;
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const boost::optional<Size>& streamPacketCount() const noexcept
    {
        return _stream.packetCount;
    }

    const boost::optional<unsigned int>& streamMajorVersion() const noexcept
    {
        return _stream.majorVersion;
    }

    const boost::optional<unsigned int>& streamMinorVersion() const noexcept
    {
        return _stream.minorVersion;
    }

    const boost::optional<yactfr::ByteOrder>& streamByteOrder() const noexcept
    {
        return _stream.byteOrder;
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
    void _setDataTypeParents();
    void _setIsCorrelatable();

private:
    const boost::filesystem::path _path;
    std::string _text;

    struct {
        boost::optional<Size> packetCount;
        boost::optional<unsigned int> majorVersion;
        boost::optional<unsigned int> minorVersion;
        boost::optional<yactfr::ByteOrder> byteOrder;
        boost::optional<boost::uuids::uuid> uuid;
    } _stream;

    yactfr::TraceType::SP _traceType;
    DataTypeParentMap _dataTypeParents;
    DataTypeScopeMap _dataTypeScopes;
    DataTypePathMap _dataTypePaths;
    Size _maxDataTypePathSize = 0;
    bool _isCorrelatable = false;
};

} // namespace jacques

#endif // _JACQUES_METADATA_HPP
