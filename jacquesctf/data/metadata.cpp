/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <memory>
#include <fstream>
#include <numeric>
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

Metadata::Metadata(bfs::path path) :
    _path {std::move(path)}
{
    try {
        std::ifstream fileStream {path.string().c_str(), std::ios::in | std::ios::binary};
        auto stream = yactfr::createMetadataStream(fileStream);

        if (auto pStream = dynamic_cast<const yactfr::PacketizedMetadataStream *>(stream.get())) {
            _stream.pktCount = pStream->packetCount();
            _stream.majorVersion = pStream->majorVersion();
            _stream.minorVersion = pStream->minorVersion();
            _stream.bo = pStream->byteOrder();
            _stream.uuid = pStream->uuid();
        }

        _text = stream->text();
        _traceType = yactfr::traceTypeFromMetadataText(_text.begin(), _text.end());
        this->_setDtParents();
        this->_setIsCorrelatable();
    } catch (const yactfr::InvalidMetadataStream& exc) {
        throw MetadataError<yactfr::InvalidMetadataStream> {path, exc};
    } catch (const yactfr::InvalidMetadata& exc) {
        throw MetadataError<yactfr::InvalidMetadata> {path, exc};
    } catch (const yactfr::MetadataParseError& exc) {
        throw MetadataError<yactfr::MetadataParseError> {path, exc};
    }
}

void Metadata::_setIsCorrelatable()
{
    if (_traceType->clockTypes().empty()) {
        return;
    }

    auto hasAbsolute = false;
    auto hasNonAbsolute = false;
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

class SetDtParentsPathsVisitor final :
    public yactfr::DataTypeVisitor
{
public:
    explicit SetDtParentsPathsVisitor(Metadata::DtParentMap& dtParentMap,
                                      Metadata::DtPathMap& dtPathMap) noexcept :
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

    void visit(const yactfr::SignedIntType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::UnsignedIntType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FloatType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::SignedEnumType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::UnsignedEnumType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::StringType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::StructType& dt) override
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, _curDtName});

        for (auto& field : dt) {
            _curDtName = &field->displayName();
            field->type().accept(*this);
            _curDtName = nullptr;
        }

        _stack.pop_back();
    }

    void visit(const yactfr::StaticArrayType& dt) override
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, _curDtName});
        _curDtName = &_arrayElemStr;
        dt.elemType().accept(*this);
        _stack.pop_back();
        _curDtName = nullptr;
    }

    void visit(const yactfr::StaticTextArrayType& dt) override
    {
        this->visit(static_cast<const yactfr::StaticArrayType&>(dt));
    }

    void visit(const yactfr::DynamicArrayType& dt) override
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, _curDtName});
        _curDtName = &_arrayElemStr;
        dt.elemType().accept(*this);
        _stack.pop_back();
        _curDtName = nullptr;
    }

    void visit(const yactfr::DynamicTextArrayType& dt) override
    {
        this->visit(static_cast<const yactfr::DynamicArrayType&>(dt));
    }

    void visit(const yactfr::VariantType& dt) override
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, _curDtName});

        for (auto& opt : dt) {
            _curDtName = &opt->displayName();
            opt->type().accept(*this);
            _curDtName = nullptr;
        }

        _stack.pop_back();
    }

private:
    static const std::string _arrayElemStr;

private:
    void _setParentAndPath(const yactfr::DataType& dt)
    {
        this->_setParent(dt);
        this->_setPath(dt);
    }

    void _setParent(const yactfr::DataType& dt)
    {
        if (_stack.empty()) {
            return;
        }

        (*_dtParentMap)[&dt] = _stack.back().parent;
    }

    void _setPath(const yactfr::DataType& dt)
    {
        if (_stack.empty()) {
            return;
        }

        auto& mapEntry = (*_dtPathMap)[&dt];

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
    Metadata::DtParentMap * const _dtParentMap;
    Metadata::DtPathMap * const _dtPathMap;
    std::vector<_StackEntry> _stack;
    const std::string *_curDtName = nullptr;
};

const std::string SetDtParentsPathsVisitor::_arrayElemStr = "%";

static void setScopeDtParents(SetDtParentsPathsVisitor& visitor, Metadata::DtScopeMap& dtScopes,
                              const yactfr::DataType *dt)
{
    if (!dt) {
        return;
    }

    dtScopes[dt] = visitor.scope();
    dt->accept(visitor);
}

void Metadata::_setDtParents()
{
    SetDtParentsPathsVisitor visitor {_dtParents, _dtPaths};

    visitor.scope(yactfr::Scope::PACKET_HEADER);
    setScopeDtParents(visitor, _dtScopes, _traceType->packetHeaderType());

    for (auto& dst : _traceType->dataStreamTypes()) {
        visitor.scope(yactfr::Scope::PACKET_CONTEXT);
        setScopeDtParents(visitor, _dtScopes, dst->packetContextType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_HEADER);
        setScopeDtParents(visitor, _dtScopes, dst->eventRecordHeaderType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_FIRST_CONTEXT);
        setScopeDtParents(visitor, _dtScopes, dst->eventRecordFirstContextType());

        for (auto& ert : dst->eventRecordTypes()) {
            visitor.scope(yactfr::Scope::EVENT_RECORD_SECOND_CONTEXT);
            setScopeDtParents(visitor, _dtScopes, ert->secondContextType());
            visitor.scope(yactfr::Scope::EVENT_RECORD_PAYLOAD);
            setScopeDtParents(visitor, _dtScopes, ert->payloadType());
        }
    }

    const auto accFunc = [](const auto total, const auto& str) {
        return total + str.size();
    };

    const auto totalSizeFunc = [accFunc](const auto& dtPathMapPair) {
        return std::accumulate(dtPathMapPair.second.path.begin(), dtPathMapPair.second.path.end(),
                               0ULL, accFunc) + dtPathMapPair.second.path.size() + 4;
    };

    const auto maxDtPathIt = std::max_element(_dtPaths.begin(), _dtPaths.end(),
                                              [totalSizeFunc](const auto& pairA,
                                                              const auto& pairB) {
        return totalSizeFunc(pairA) < totalSizeFunc(pairB);
    });

    _maxDtPathSize = maxDtPathIt == _dtPaths.end() ? 0 : totalSizeFunc(*maxDtPathIt);
}

const Metadata::DtPath& Metadata::dtPath(const yactfr::DataType& dt) const noexcept
{
    return _dtPaths.find(&dt)->second;
}

const yactfr::DataType *Metadata::dtParent(const yactfr::DataType& dt) const noexcept
{
    const auto it = _dtParents.find(&dt);

    if (it == _dtParents.end()) {
        return nullptr;
    }

    return it->second;
}

yactfr::Scope Metadata::dtScope(const yactfr::DataType& dt) const noexcept
{
    auto curDt = &dt;

    while (!this->dtIsScopeRoot(*curDt)) {
        curDt = this->dtParent(*curDt);
        assert(curDt);
    }

    return _dtScopes.find(curDt)->second;
}

bool Metadata::dtIsScopeRoot(const yactfr::DataType& dt) const noexcept
{
    return _dtScopes.find(&dt) != _dtScopes.end();
}

DataLen Metadata::fileLen() const noexcept
{
    return DataLen::fromBytes(bfs::file_size(_path));
}

} // namespace jacques
