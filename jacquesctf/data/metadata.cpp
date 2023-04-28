/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <memory>
#include <fstream>
#include <numeric>
#include <yactfr/yactfr.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "metadata.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

Metadata::Metadata(boost::filesystem::path path, yactfr::TraceType::UP traceType,
                   std::unique_ptr<const yactfr::MetadataStream> stream,
                   boost::optional<boost::uuids::uuid> streamUuid) :
    _path {std::move(path)},
    _traceType {std::move(traceType)},
    _stream {std::move(stream)},
    _streamUuid {std::move(streamUuid)}
{
    this->_setDtParents();
    this->_setIsCorrelatable();
}

void Metadata::_setIsCorrelatable()
{
    if (_traceType->clockTypes().empty()) {
        return;
    }

    auto& firstClkTypeOrig = (*_traceType->clockTypes().begin())->origin();

    for (auto& clkType : _traceType->clockTypes()) {
        if (!clkType->origin()) {
            return;
        }

        if (*clkType->origin() != *firstClkTypeOrig) {
            return;
        }
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

    void visit(const yactfr::FixedLengthBitArrayType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthBooleanType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthFloatingPointNumberType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthUnsignedIntegerType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthSignedIntegerType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthUnsignedEnumerationType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::FixedLengthSignedEnumerationType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::VariableLengthUnsignedIntegerType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::VariableLengthSignedIntegerType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::VariableLengthUnsignedEnumerationType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::VariableLengthSignedEnumerationType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::NullTerminatedStringType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::StaticLengthStringType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::DynamicLengthStringType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::StaticLengthBlobType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::DynamicLengthBlobType& dt) override
    {
        this->_setParentAndPath(dt);
    }

    void visit(const yactfr::StructureType& dt) override
    {
        this->_setParentAndPath(dt);

        for (Index i = 0; i < dt.size(); ++i) {
            auto& memberType = dt[i];

            _stack.push_back({&dt, DtPath::StructMemberItem {i, *memberType.displayName()}});
            memberType.dataType().accept(*this);
            _stack.pop_back();
        }
    }

    void visit(const yactfr::StaticLengthArrayType& dt) override
    {
        this->_visitArrayType(dt);
    }

    void visit(const yactfr::DynamicLengthArrayType& dt) override
    {
        this->_visitArrayType(dt);
    }

    void visit(const yactfr::OptionalWithBooleanSelectorType& dt) override
    {
        this->_visitOptType(dt);
    }

    void visit(const yactfr::OptionalWithUnsignedIntegerSelectorType& dt) override
    {
        this->_visitOptType(dt);
    }

    void visit(const yactfr::OptionalWithSignedIntegerSelectorType& dt) override
    {
        this->_visitOptType(dt);
    }

    void visit(const yactfr::VariantWithUnsignedIntegerSelectorType& dt) override
    {
        this->_visitVarType(dt);
    }

    void visit(const yactfr::VariantWithSignedIntegerSelectorType& dt) override
    {
        this->_visitVarType(dt);
    }

private:
    template <typename VarTypeT>
    void _visitVarType(VarTypeT& dt)
    {
        this->_setParentAndPath(dt);

        for (Index i = 0; i < dt.size(); ++i) {
            auto& opt = dt[i];

            _stack.push_back({&dt, DtPath::VarOptItem {i, opt.displayName()}});
            opt.dataType().accept(*this);
            _stack.pop_back();
        }
    }

    void _visitOptType(const yactfr::OptionalType& dt)
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, DtPath::CurOptDataItem {}});
        dt.dataType().accept(*this);
        _stack.pop_back();
    }

    void _visitArrayType(const yactfr::ArrayType& dt)
    {
        this->_setParentAndPath(dt);
        _stack.push_back({&dt, DtPath::CurArrayElemItem {}});
        dt.elementType().accept(*this);
        _stack.pop_back();
    }

    void _setParentAndPath(const yactfr::DataType& dt)
    {
        this->_setParent(dt);
        this->_setPath(dt);
    }

    void _setParent(const yactfr::DataType& dt)
    {
        if (_stack.empty()) {
            // root
            return;
        }

        (*_dtParentMap)[&dt] = _stack.back().parentDt;
    }

    void _setPath(const yactfr::DataType& dt)
    {
        DtPath::Items items;

        std::transform(_stack.cbegin(), _stack.cend(), std::back_inserter(items),
                       [](auto& entry) {
            return entry.pathItem;
        });

        _dtPathMap->insert(std::make_pair(&dt, DtPath {_scope, std::move(items)}));
    }

private:
    struct _StackEntry
    {
        const yactfr::DataType *parentDt;
        const DtPath::Item pathItem;
    };

private:
    yactfr::Scope _scope;
    Metadata::DtParentMap * const _dtParentMap;
    Metadata::DtPathMap * const _dtPathMap;
    std::vector<_StackEntry> _stack;
};

namespace {

void setScopeDtParentsPaths(SetDtParentsPathsVisitor& visitor, Metadata::DtScopeMap& dtScopes,
                            const yactfr::DataType *dt)
{
    if (!dt) {
        return;
    }

    dtScopes[dt] = visitor.scope();
    dt->accept(visitor);
}

} // namespace

void Metadata::_setDtParents()
{
    SetDtParentsPathsVisitor visitor {_dtParents, _dtPaths};

    visitor.scope(yactfr::Scope::PACKET_HEADER);
    setScopeDtParentsPaths(visitor, _dtScopes, _traceType->packetHeaderType());

    for (auto& dst : _traceType->dataStreamTypes()) {
        visitor.scope(yactfr::Scope::PACKET_CONTEXT);
        setScopeDtParentsPaths(visitor, _dtScopes, dst->packetContextType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_HEADER);
        setScopeDtParentsPaths(visitor, _dtScopes, dst->eventRecordHeaderType());
        visitor.scope(yactfr::Scope::EVENT_RECORD_COMMON_CONTEXT);
        setScopeDtParentsPaths(visitor, _dtScopes, dst->eventRecordCommonContextType());

        for (auto& ert : dst->eventRecordTypes()) {
            visitor.scope(yactfr::Scope::EVENT_RECORD_SPECIFIC_CONTEXT);
            setScopeDtParentsPaths(visitor, _dtScopes, ert->specificContextType());
            visitor.scope(yactfr::Scope::EVENT_RECORD_PAYLOAD);
            setScopeDtParentsPaths(visitor, _dtScopes, ert->payloadType());
        }
    }

#if 0
    const auto accFunc = [](const auto total, const auto& str) {
        return total + str.size();
    };

    const auto totalSizeFunc = [accFunc](const auto& dtPathMapPair) {
        return std::accumulate(dtPathMapPair.second.path.begin(), dtPathMapPair.second.path.end(),
                               0ULL, accFunc) + dtPathMapPair.second.path.size() + 4;
    };

    const auto maxDtPathIt = std::max_element(_dtPaths.begin(), _dtPaths.end(),
                                              [totalSizeFunc](const auto& pairA, const auto& pairB) {
        return totalSizeFunc(pairA) < totalSizeFunc(pairB);
    });

    _maxDtPathSize = maxDtPathIt == _dtPaths.end() ? 0 : totalSizeFunc(*maxDtPathIt);
#endif
}

const DtPath& Metadata::dtPath(const yactfr::DataType& dt) const noexcept
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
