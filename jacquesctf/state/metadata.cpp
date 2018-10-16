/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <memory>
#include <fstream>
#include <yactfr/metadata/clock-type.hpp>
#include <yactfr/metadata/metadata-stream.hpp>
#include <yactfr/metadata/packetized-metadata-stream.hpp>
#include <yactfr/metadata/data-type-visitor.hpp>
#include <yactfr/metadata/trace-type-from-metadata-text.hpp>
#include <yactfr/metadata/variant-type.hpp>
#include <yactfr/metadata/dynamic-array-type.hpp>
#include <yactfr/metadata/static-array-type.hpp>
#include <yactfr/metadata/struct-type.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "metadata.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

Metadata::Metadata(const bfs::path& path) :
    _path {path}
{
    try {
        std::ifstream fileStream {path.string().c_str(),
                                  std::ios::in | std::ios::binary};
        auto stream = yactfr::createMetadataStream(fileStream);

        if (auto pStream = dynamic_cast<const yactfr::PacketizedMetadataStream *>(stream.get())) {
            _stream.packetCount = pStream->packetCount();
            _stream.majorVersion = pStream->majorVersion();
            _stream.minorVersion = pStream->minorVersion();
            _stream.byteOrder = pStream->byteOrder();
            _stream.uuid = pStream->uuid();
        }

        _traceType = yactfr::traceTypeFromMetadataText(std::begin(stream->text()),
                                                       std::end(stream->text()));
        this->_setDataTypeParents();
        this->_setIsCorrelatable();
    } catch (const yactfr::InvalidMetadataStream& ex) {
        _invalidStreamError = ex;
    } catch (const yactfr::InvalidMetadata& ex) {
        _invalidMetadataError = ex;
    } catch (const yactfr::MetadataParseError& ex) {
        _parseError = ex;
    }
}

void Metadata::_setIsCorrelatable()
{
    if (_traceType->clockTypes().empty()) {
        return;
    }

    bool hasAbsolute = false;
    bool hasNonAbsolute = false;
    const boost::uuids::uuid *uuid = nullptr;

    for (auto& clockType : _traceType->clockTypes()) {
        if (clockType->isAbsolute()) {
            hasAbsolute = true;
        } else {
            if (uuid) {
                if (clockType->uuid()) {
                    if (*clockType->uuid() != *uuid) {
                        return;
                    }
                } else {
                    return;
                }
            } else {
                if (clockType->uuid()) {
                    uuid = &(*clockType->uuid());
                }
            }

            hasNonAbsolute = true;
        }
    }

    if (hasAbsolute && hasNonAbsolute) {
        return;
    }

    _isCorrelatable = true;
}

class SetDataTypeParentsPathsVisitor :
    public yactfr::DataTypeVisitor
{
public:
    explicit SetDataTypeParentsPathsVisitor(Metadata::DataTypeParentMap& dtParentMap,
                                            Metadata::DataTypePathMap& dtPathMap) :
        _dtParentMap {&dtParentMap},
        _dtPathMap {&dtPathMap}
    {
    }

    void scope(const yactfr::Scope scope) noexcept
    {
        _scope = scope;
    }

    yactfr::Scope scope() const noexcept
    {
        return _scope;
    }

    void visit(const yactfr::SignedIntType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::UnsignedIntType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::FloatType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::SignedEnumType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::UnsignedEnumType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::StringType& type) override
    {
        this->_setParentAndPath(type);
    }

    void visit(const yactfr::StructType& type) override
    {
        this->_setParentAndPath(type);
        _stack.push_back({&type, _curDtName});

        for (auto& field : type) {
            _curDtName = &field->displayName();
            field->type().accept(*this);
            _curDtName = nullptr;
        }

        _stack.pop_back();
    }

    void visit(const yactfr::StaticArrayType& type) override
    {
        this->_setParentAndPath(type);
        _stack.push_back({&type, &_arrayElemStr});
        type.elemType().accept(*this);
        _stack.pop_back();
    }

    void visit(const yactfr::StaticTextArrayType& type) override
    {
        this->visit(static_cast<const yactfr::StaticArrayType&>(type));
    }

    void visit(const yactfr::DynamicArrayType& type) override
    {
        this->_setParentAndPath(type);
        _stack.push_back({&type, &_arrayElemStr});
        type.elemType().accept(*this);
        _stack.pop_back();
    }

    void visit(const yactfr::DynamicTextArrayType& type) override
    {
        this->visit(static_cast<const yactfr::DynamicArrayType&>(type));
    }

    void visit(const yactfr::VariantType& type) override
    {
        this->_setParentAndPath(type);
        _stack.push_back({&type, _curDtName});

        for (auto& opt : type) {
            _curDtName = &opt->displayName();
            opt->type().accept(*this);
            _curDtName = nullptr;
        }

        _stack.pop_back();
    }

private:
    static const std::string _arrayElemStr;

private:
    void _setParentAndPath(const yactfr::DataType& dataType)
    {
        this->_setParent(dataType);
        this->_setPath(dataType);
    }

    void _setParent(const yactfr::DataType& dataType)
    {
        if (_stack.empty()) {
            return;
        }

        (*_dtParentMap)[&dataType] = _stack.back().parent;
    }

    void _setPath(const yactfr::DataType& dataType)
    {
        if (_stack.empty()) {
            return;
        }

        auto& mapEntry = (*_dtPathMap)[&dataType];

        mapEntry.scope = _scope;

        for (const auto& entry : _stack) {
            if (entry.parentName) {
                mapEntry.path.push_back(*entry.parentName);
            }
        }

        if (_curDtName) {
            mapEntry.path.push_back(*_curDtName);
        }
    }

private:
    struct _StackEntry
    {
        const yactfr::DataType *parent;
        const std::string *parentName;
    };

private:
    yactfr::Scope _scope;
    Metadata::DataTypeParentMap * const _dtParentMap;
    Metadata::DataTypePathMap * const _dtPathMap;
    std::vector<_StackEntry> _stack;
    const std::string *_curDtName = nullptr;
};

const std::string SetDataTypeParentsPathsVisitor::_arrayElemStr = "%";

static void setScopeDataTypeParents(SetDataTypeParentsPathsVisitor& visitor,
                                    Metadata::DataTypeScopeMap& dataTypeScopes,
                                    const yactfr::DataType *dataType)
{
    if (!dataType) {
        return;
    }

    dataTypeScopes[dataType] = visitor.scope();
    dataType->accept(visitor);
}

void Metadata::_setDataTypeParents()
{
    SetDataTypeParentsPathsVisitor visitor {_dataTypeParents,
                                            _dataTypePaths};

    visitor.scope(yactfr::Scope::PACKET_HEADER);
    setScopeDataTypeParents(visitor, _dataTypeScopes,
                            _traceType->packetHeaderType());

    for (auto& dst : _traceType->dataStreamTypes()) {
        visitor.scope(yactfr::Scope::PACKET_CONTEXT);
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->packetContextType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_HEADER);
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->eventRecordHeaderType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT);
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->eventRecordFirstContextType());

        for (auto& ert : dst->eventRecordTypes()) {
            visitor.scope(yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT);
            setScopeDataTypeParents(visitor, _dataTypeScopes,
                                    ert->secondContextType());
            visitor.scope(yactfr::Scope::EVENT_RECORD_PAYLOAD);
            setScopeDataTypeParents(visitor, _dataTypeScopes,
                                    ert->payloadType());
        }
    }
}

const Metadata::DataTypePath& Metadata::dataTypePath(const yactfr::DataType& dataType) const
{
    return _dataTypePaths.find(&dataType)->second;
}

const yactfr::DataType *Metadata::dataTypeParent(const yactfr::DataType& dataType) const
{
    auto it = _dataTypeParents.find(&dataType);

    if (it == std::end(_dataTypeParents)) {
        return nullptr;
    }

    return it->second;
}

yactfr::Scope Metadata::dataTypeScope(const yactfr::DataType& dataType) const
{
    const yactfr::DataType *curDataType = &dataType;

    while (!this->dataTypeIsScopeRoot(*curDataType)) {
        curDataType = this->dataTypeParent(*curDataType);
        assert(curDataType);
    }

    return _dataTypeScopes.find(curDataType)->second;
}

bool Metadata::dataTypeIsScopeRoot(const yactfr::DataType& dataType) const
{
    auto it = _dataTypeScopes.find(&dataType);

    if (it == std::end(_dataTypeScopes)) {
        return false;
    }

    return true;
}

DataSize Metadata::fileSize() const noexcept
{
    return DataSize::fromBytes(bfs::file_size(_path));
}

} // namespace jacques
