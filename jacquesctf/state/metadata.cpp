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

    for (auto& clockType : _traceType->clockTypes()) {
        if (clockType->isAbsolute()) {
            hasAbsolute = true;
        } else {
            hasNonAbsolute = true;
        }
    }

    if (hasAbsolute && hasNonAbsolute) {
        return;
    }

    _isCorrelatable = true;
}

class SetDataTypeParentsVisitor :
    public yactfr::DataTypeVisitor
{
public:
    explicit SetDataTypeParentsVisitor(Metadata::DataTypeParentMap& map) :
        _map {&map}
    {
    }

    void visit(const yactfr::SignedIntType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::UnsignedIntType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::FloatType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::SignedEnumType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::UnsignedEnumType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::StringType& type) override
    {
        this->_setParent(type);
    }

    void visit(const yactfr::StructType& type) override
    {
        this->_setParent(type);
        _stack.push_back(&type);

        for (auto& field : type) {
            field->type().accept(*this);
        }

        _stack.pop_back();
    }

    void visit(const yactfr::StaticArrayType& type) override
    {
        this->_setParent(type);
        _stack.push_back(&type);
        type.elemType().accept(*this);
        _stack.pop_back();
    }

    void visit(const yactfr::StaticTextArrayType& type) override
    {
        this->visit(static_cast<const yactfr::StaticArrayType&>(type));
    }

    void visit(const yactfr::DynamicArrayType& type) override
    {
        this->_setParent(type);
        _stack.push_back(&type);
        type.elemType().accept(*this);
        _stack.pop_back();
    }

    void visit(const yactfr::DynamicTextArrayType& type) override
    {
        this->visit(static_cast<const yactfr::DynamicArrayType&>(type));
    }

    void visit(const yactfr::VariantType& type) override
    {
        this->_setParent(type);
        _stack.push_back(&type);

        for (auto& choice : type) {
            choice->type().accept(*this);
        }

        _stack.pop_back();
    }

private:
    void _setParent(const yactfr::DataType& dataType)
    {
        if (_stack.empty()) {
            return;
        }

        (*_map)[&dataType] = _stack.back();
    }

private:
    Metadata::DataTypeParentMap *_map;
    std::vector<const yactfr::DataType *> _stack;
};

static void setScopeDataTypeParents(SetDataTypeParentsVisitor& visitor,
                                    Metadata::DataTypeScopeMap& dataTypeScopes,
                                    const yactfr::DataType *dataType,
                                    const yactfr::Scope scope)
{
    if (!dataType) {
        return;
    }

    dataTypeScopes[dataType] = scope;
    dataType->accept(visitor);
}

void Metadata::_setDataTypeParents()
{
    SetDataTypeParentsVisitor visitor {_dataTypeParents};

    setScopeDataTypeParents(visitor, _dataTypeScopes,
                            _traceType->packetHeaderType(),
                            yactfr::Scope::PACKET_HEADER);

    for (auto& dst : _traceType->dataStreamTypes()) {
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->packetContextType(),
                                yactfr::Scope::PACKET_CONTEXT);
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->eventRecordHeaderType(),
                                yactfr::Scope::EVENT_RECORD_HEADER);
        setScopeDataTypeParents(visitor, _dataTypeScopes,
                                dst->eventRecordFirstContextType(),
                                yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT);

        for (auto& ert : dst->eventRecordTypes()) {
            setScopeDataTypeParents(visitor, _dataTypeScopes,
                                    ert->secondContextType(),
                                    yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT);
            setScopeDataTypeParents(visitor, _dataTypeScopes,
                                    ert->payloadType(),
                                    yactfr::Scope::EVENT_RECORD_PAYLOAD);
        }
    }
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
