/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_DS_FILE_HPP
#define _JACQUES_DATA_DS_FILE_HPP

#include <cassert>
#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/core/noncopyable.hpp>
#include <yactfr/element-sequence.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/memory-mapped-file-view-factory.hpp>

#include "aliases.hpp"
#include "pkt.hpp"
#include "pkt-index-entry.hpp"
#include "metadata.hpp"
#include "data-len.hpp"
#include "pkt-checkpoints-build-listener.hpp"

namespace jacques {

class DsFile final :
    boost::noncopyable
{
public:
    using BuildIndexProgressFunc = std::function<void (const PktIndexEntry&)>;

public:
    explicit DsFile(boost::filesystem::path path, const Metadata& metadata);
    ~DsFile();
    void buildIndex();
    void buildIndex(const BuildIndexProgressFunc& progressFunc, Size step = 1);
    bool hasOffsetBits(Index offsetBits) const noexcept;
    Pkt& pktAtIndex(Index index, PktCheckpointsBuildListener& buildListener);
    const PktIndexEntry& pktIndexEntryContainingOffsetBits(Index offsetBits) const noexcept;
    const PktIndexEntry *pktIndexEntryWithSeqNum(Index seqNum) const noexcept;
    const PktIndexEntry *pktIndexEntryContainingNsFromOrigin(long long nsFromOrigin) const noexcept;
    const PktIndexEntry *pktIndexEntryContainingCycles(unsigned long long cycles) const noexcept;

    Size pktCount() const noexcept
    {
        assert(_isIndexBuilt);
        return _index.size();
    }

    const PktIndexEntry& pktIndexEntry(const Index index) const noexcept
    {
        assert(_isIndexBuilt);
        assert(index < _index.size());
        return _index[index];
    }

    PktIndexEntry& pktIndexEntry(const Index index) noexcept
    {
        assert(_isIndexBuilt);
        assert(index < _index.size());
        return _index[index];
    }

    const std::vector<PktIndexEntry>& pktIndexEntries() const noexcept
    {
        assert(_isIndexBuilt);
        return _index;
    }

    std::vector<PktIndexEntry>& pktIndexEntries() noexcept
    {
        assert(_isIndexBuilt);
        return _index;
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const DataLen& fileLen() const noexcept
    {
        return _fileLen;
    }

    bool hasError() const noexcept
    {
        return _hasError;
    }

    const Metadata& metadata() const noexcept
    {
        return *_metadata;
    }

private:
    struct _IndexBuildingState
    {
        void reset();

        boost::optional<Index> pktCtxOffsetInPktBits;
        boost::optional<DataLen> preambleLen;
        boost::optional<DataLen> expectedTotalLen;
        boost::optional<DataLen> expectedContentLen;
        boost::optional<Ts> beginTs;
        boost::optional<Ts> endTs;
        boost::optional<Index> dsId;
        boost::optional<Index> seqNum;
        boost::optional<Size> discardedErCounter;
        const yactfr::DataStreamType *dst = nullptr;
        bool inPktCtxScope = false;
    };

private:
    void _buildIndex(const BuildIndexProgressFunc& progressFunc, Size step);

    void _addPktIndexEntry(Index offsetInDsFileBytes, Index offsetInDsFileBits,
                           const _IndexBuildingState& state, bool isInvalid);

    template <typename TsLtCompFuncT, typename ValInTsFuncT, typename ValT>
    const PktIndexEntry *_pktIndexEntryContainingVal(TsLtCompFuncT&& tsLtCompFunc,
                                                     ValInTsFuncT&& valInTsFunc,
                                                     const ValT val) const noexcept
    {
        if (!_metadata->isCorrelatable()) {
            return nullptr;
        }

        if (_index.empty()) {
            return nullptr;
        }

        auto it = std::lower_bound(_index.begin(), _index.end(), val,
                                   [tsLtCompFunc](const auto& entry, const auto val) {
            if (!entry.beginTs()) {
                return false;
            }

            return std::forward<TsLtCompFuncT>(tsLtCompFunc)(*entry.beginTs(), val);
        });

        if (it == _index.end()) {
            --it;
        }

        if (!it->beginTs() || !it->endTs()) {
            return nullptr;
        }

        if (!std::forward<ValInTsFuncT>(valInTsFunc)(val, *it)) {
            if (it == _index.begin()) {
                return nullptr;
            }

            --it;

            if (!it->beginTs() || !it->endTs()) {
                return nullptr;
            }

            if (!std::forward<ValInTsFuncT>(valInTsFunc)(val, *it)) {
                return nullptr;
            }
        }

        return &*it;
    }

private:
    const boost::filesystem::path _path;
    const Metadata * const _metadata;
    std::shared_ptr<yactfr::MemoryMappedFileViewFactory> _factory;
    yactfr::ElementSequence _seq;
    DataLen _fileLen;
    std::vector<PktIndexEntry> _index;
    std::vector<std::unique_ptr<Pkt>> _pkts;
    int _fd;
    bool _isIndexBuilt = false;
    bool _hasError = false;
};

} // namespace jacques

#endif // _JACQUES_DATA_DS_FILE_HPP
