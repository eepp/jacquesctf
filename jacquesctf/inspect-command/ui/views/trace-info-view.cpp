/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/variant/get.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <yactfr/metadata/trace-type.hpp>
#include <yactfr/metadata/trace-type-env.hpp>
#include <yactfr/metadata/data-stream-type.hpp>
#include <yactfr/metadata/clock-type.hpp>

#include "trace-info-view.hpp"
#include "data/timestamp.hpp"
#include "data/time-ops.hpp"
#include "../stylist.hpp"
#include "../../state/message.hpp"

namespace jacques {

TraceInfoView::_Row::~_Row()
{
}

TraceInfoView::TraceInfoView(const Rectangle& rect,
                             const Stylist& stylist, State& state) :
    ScrollView {rect, "Trace info", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_buildRows();
    _rows = &_traceInfo[state.metadata().traceType().get()];
    this->_rowCount(_rows->size());
    this->_drawRows();
}

void TraceInfoView::_buildTraceInfoRows(const Metadata &metadata)
{
    auto& traceType = *metadata.traceType();

    if (_traceInfo.find(&traceType) != std::end(_traceInfo)) {
        return;
    }

    Rows rows;

    rows.push_back(std::make_unique<_SectionRow>("Paths"));
    rows.push_back(std::make_unique<_StringPropRow>("Trace directory", metadata.path().parent_path().string()));
    rows.push_back(std::make_unique<_StringPropRow>("Metadata stream", metadata.path().string()));
    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Data stream info"));

    Size dsfCount = 0;
    Size dsfWithoutDsIdCount = 0;
    DataSize totalEffectivePacketsTotalSize = 0;
    DataSize totalEffectivePacketsContentSize = 0;
    DataSize totalExpectedPacketsTotalSize = 0;
    DataSize totalExpectedPacketsContentSize = 0;
    boost::optional<Timestamp> firstTs;
    boost::optional<Timestamp> lastTs;
    boost::optional<Timestamp> intersectFirstTs;
    boost::optional<Timestamp> intersectLastTs;
    Size packetCount = 0;
    std::set<std::pair<Index, Index>> dataStreamIds;

    for (const auto& dsfState : _state->dataStreamFileStates()) {
        auto dsfStateTraceType = dsfState->metadata().traceType().get();

        if (dsfStateTraceType != &traceType) {
            continue;
        }

        const auto& dsFile = dsfState->dataStreamFile();

        ++dsfCount;
        packetCount += dsFile.packetCount();

        for (const auto& entry : dsFile.packetIndexEntries()) {
            if (entry.expectedContentSize()) {
                totalExpectedPacketsContentSize += *entry.expectedContentSize();
            }

            if (entry.expectedTotalSize()) {
                totalExpectedPacketsTotalSize += *entry.expectedTotalSize();
            }

            totalEffectivePacketsContentSize += entry.effectiveContentSize();
            totalEffectivePacketsTotalSize += entry.effectiveTotalSize();
        }

        if (dsFile.packetCount() > 0) {
            const auto &dataStreamId = dsFile.packetIndexEntry(0).dataStreamId();
            const auto dst = dsFile.packetIndexEntry(0).dataStreamType();

            if (dataStreamId && dst) {
                dataStreamIds.insert({dst->id(), *dataStreamId});
            } else {
                ++dsfWithoutDsIdCount;
            }

            const auto& firstPacketIndexEntry = dsFile.packetIndexEntries().front();
            const auto& lastPacketIndexEntry = dsFile.packetIndexEntries().back();
            const auto& dsfFirstTs = firstPacketIndexEntry.beginningTimestamp();
            const auto& dsfLastTs = lastPacketIndexEntry.endTimestamp();

            if (dsfFirstTs) {
                if (!firstTs || *dsfFirstTs < *firstTs) {
                    firstTs = *dsfFirstTs;
                }

                if (!intersectFirstTs || *dsfFirstTs > *intersectFirstTs) {
                    intersectFirstTs = *dsfFirstTs;
                }
            }

            if (dsfLastTs) {
                if (!lastTs || *dsfLastTs > *lastTs) {
                    lastTs = *dsfLastTs;
                }

                if (!intersectLastTs || *dsfLastTs < *intersectLastTs) {
                    intersectLastTs = *dsfLastTs;
                }
            }
        }
    }

    rows.push_back(std::make_unique<_SignedIntPropRow>("Data stream files",
                                                       static_cast<long long>(dsfCount)));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Data streams",
                                                       static_cast<long long>(dataStreamIds.size() + dsfWithoutDsIdCount)));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Packets",
                                                       static_cast<long long>(packetCount)));
    rows.push_back(std::make_unique<_SepRow>());
    rows.push_back(std::make_unique<_DataSizePropRow>("Effective packets's total size",
                                                      totalEffectivePacketsTotalSize));
    rows.push_back(std::make_unique<_DataSizePropRow>("Effective packets's total content size",
                                                      totalEffectivePacketsContentSize));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Effective packets's total content size (b)",
                                                       static_cast<long long>(totalEffectivePacketsContentSize.bits())));

    const auto totalEffectivePacketsPaddingSize = totalEffectivePacketsTotalSize -
                                                  totalEffectivePacketsContentSize;

    rows.push_back(std::make_unique<_DataSizePropRow>("Effective packets's total padding size",
                                                      totalEffectivePacketsPaddingSize));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Effective packets's total padding size (b)",
                                                       static_cast<long long>(totalEffectivePacketsPaddingSize.bits())));
    rows.push_back(std::make_unique<_SepRow>());
    rows.push_back(std::make_unique<_DataSizePropRow>("Expected packets's total size",
                                                      totalExpectedPacketsTotalSize));
    rows.push_back(std::make_unique<_DataSizePropRow>("Expected packets's total content size",
                                                      totalExpectedPacketsContentSize));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Expected packets's total content size (b)",
                                                       static_cast<long long>(totalExpectedPacketsContentSize.bits())));

    const auto totalExpectedPacketsPaddingSize = totalExpectedPacketsTotalSize -
                                                 totalExpectedPacketsContentSize;

    rows.push_back(std::make_unique<_DataSizePropRow>("Expected packets's total padding size",
                                                      totalExpectedPacketsPaddingSize));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Expected packets's total padding size (b)",
                                                       static_cast<long long>(totalExpectedPacketsPaddingSize.bits())));
    rows.push_back(std::make_unique<_SepRow>());


    if (firstTs) {
        rows.push_back(std::make_unique<_TimestampPropRow>("Beginning", *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Beginning"));
    }

    if (lastTs) {
        rows.push_back(std::make_unique<_TimestampPropRow>("End", *lastTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("End"));
    }

    if (firstTs && lastTs && firstTs <= lastTs) {
        rows.push_back(std::make_unique<_DurationPropRow>("Duration",
                                                          *lastTs - *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Duration"));
    }

    rows.push_back(std::make_unique<_SepRow>());

    if (intersectFirstTs && intersectLastTs &&
            *intersectFirstTs < *intersectLastTs) {
        rows.push_back(std::make_unique<_TimestampPropRow>("Intersection beginning", *intersectFirstTs));
        rows.push_back(std::make_unique<_TimestampPropRow>("Intersection end", *intersectLastTs));

        const auto intersectDuration = *intersectLastTs - *intersectFirstTs;

        rows.push_back(std::make_unique<_DurationPropRow>("Intersection duration",
                                                          intersectDuration));

        assert(firstTs);
        assert(lastTs);
        rows.push_back(std::make_unique<_SepRow>());

        const auto disjointDuration = Duration {
            (*lastTs - *firstTs).ns() - intersectDuration.ns()
        };

        rows.push_back(std::make_unique<_DurationPropRow>("Beginning to intersection",
                                                          *intersectFirstTs - *firstTs));
        rows.push_back(std::make_unique<_DurationPropRow>("Intersection to end",
                                                          *lastTs - *intersectLastTs));
        rows.push_back(std::make_unique<_DurationPropRow>("Disjoint duration",
                                                          disjointDuration));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Intersection"));
    }

    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Metadata stream info"));

    rows.push_back(std::make_unique<_StringPropRow>("Packetized",
                                                    metadata.streamPacketCount() ? "Yes" : "No"));
    rows.push_back(std::make_unique<_StringPropRow>("Path", metadata.path().string()));
    rows.push_back(std::make_unique<_DataSizePropRow>("Size", metadata.fileSize()));

    if (metadata.streamPacketCount()) {
        const auto version = std::to_string(*metadata.streamMajorVersion()) + "." +
                             std::to_string(*metadata.streamMinorVersion());

        rows.push_back(std::make_unique<_StringPropRow>("Version", version));
        rows.push_back(std::make_unique<_SignedIntPropRow>("Packets",
                                                           static_cast<long long>(*metadata.streamPacketCount())));

        auto byteOrder = (*metadata.streamByteOrder() == yactfr::ByteOrder::BIG) ? "Big endian" : "Little endian";

        rows.push_back(std::make_unique<_StringPropRow>("Byte order", byteOrder));
        rows.push_back(std::make_unique<_StringPropRow>("UUID",
                                                        boost::uuids::to_string(*metadata.streamUuid())));
    }

    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Trace type"));

    const auto version = std::to_string(traceType.majorVersion()) + "." +
                         std::to_string(traceType.minorVersion());

    rows.push_back(std::make_unique<_StringPropRow>("Version", version));

    if (traceType.uuid()) {
        rows.push_back(std::make_unique<_StringPropRow>("UUID",
                                                        boost::uuids::to_string(*traceType.uuid())));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("UUID"));
    }

    rows.push_back(std::make_unique<_StringPropRow>("Is correlatable",
                                                    metadata.isCorrelatable() ? "Yes" : "No"));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Data stream types",
                                                       static_cast<long long>(traceType.dataStreamTypes().size())));

    const auto ertCount = std::accumulate(std::begin(traceType.dataStreamTypes()),
                                          std::end(traceType.dataStreamTypes()),
                                          0, [](const auto curCount, const auto& dst) {
        return curCount + dst->eventRecordTypes().size();
    });

    rows.push_back(std::make_unique<_SignedIntPropRow>("Event record types",
                                                       static_cast<long long>(ertCount)));
    rows.push_back(std::make_unique<_SignedIntPropRow>("Clock types",
                                                       static_cast<long long>(traceType.clockTypes().size())));

    if (!traceType.env().entries().empty()) {
        rows.push_back(std::make_unique<_EmptyRow>());
        rows.push_back(std::make_unique<_SectionRow>("Trace type environment"));

        std::vector<const yactfr::TraceTypeEnv::Entries::value_type *> entries;

        for (auto& entryPair : traceType.env().entries()) {
            entries.push_back(&entryPair);
        }

        std::sort(std::begin(entries), std::end(entries),
                  [](const auto entryLeft, const auto entryRight) {
            return entryLeft->first < entryRight->first;
        });

        for (const auto entryPair : entries) {
            if (const auto intEntry = boost::get<long long>(&entryPair->second)) {
                rows.push_back(std::make_unique<_SignedIntPropRow>(entryPair->first,
                                                                   *intEntry));
            } else if (const auto strEntry = boost::get<std::string>(&entryPair->second)) {
                rows.push_back(std::make_unique<_StringPropRow>(entryPair->first,
                                                                *strEntry));
            }
        }
    }

    for (auto& dst : traceType.dataStreamTypes()) {
        rows.push_back(std::make_unique<_EmptyRow>());

        const auto title = std::string {"Data stream type "} +
                           std::to_string(dst->id());

        rows.push_back(std::make_unique<_SectionRow>(title));
        rows.push_back(std::make_unique<_SignedIntPropRow>("Event record types",
                                                           static_cast<long long>(dst->eventRecordTypes().size())));
    }

    for (auto& clockType : traceType.clockTypes()) {
        rows.push_back(std::make_unique<_EmptyRow>());

        const auto title = std::string {"Clock type `"} +
                           clockType->name() + "`";

        rows.push_back(std::make_unique<_SectionRow>(title));

        if (clockType->description()) {
            rows.push_back(std::make_unique<_StringPropRow>("Description",
                                                            *clockType->description()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>("Description"));
        }

        if (clockType->uuid()) {
            rows.push_back(std::make_unique<_StringPropRow>("UUID",
                                                            boost::uuids::to_string(*clockType->uuid())));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>("UUID"));
        }

        rows.push_back(std::make_unique<_SignedIntPropRow>("Frequency (Hz)",
                                                           static_cast<long long>(clockType->freq())));
        rows.push_back(std::make_unique<_SignedIntPropRow>("Offset (s)",
                                                           static_cast<long long>(clockType->offset().seconds())));
        rows.push_back(std::make_unique<_SignedIntPropRow>("Offset (cycles)",
                                                           static_cast<long long>(clockType->offset().cycles())));
        rows.push_back(std::make_unique<_SignedIntPropRow>("Error (cycles)",
                                                           static_cast<long long>(clockType->error())));
        rows.push_back(std::make_unique<_StringPropRow>("Is absolute",
                                                        clockType->isAbsolute() ? "Yes" : "No"));
    }

    _traceInfo[&traceType] = std::move(rows);
}

void TraceInfoView::_buildRows()
{
    for (const auto& dsfState : _state->dataStreamFileStates()) {
        this->_buildTraceInfoRows(dsfState->metadata());
    }

    for (auto& traceInfo : _traceInfo) {
        Size longestKeySize = 0;
        auto it = std::begin(traceInfo.second);
        auto lastSectionIt = it;
        const auto setValueOffsets = [&longestKeySize, &lastSectionIt, &it]() {
            while (lastSectionIt != it) {
                if (auto sRow = dynamic_cast<_PropRow *>(lastSectionIt->get())) {
                    sRow->valueOffset = longestKeySize + 2;
                }

                ++lastSectionIt;
            }

            longestKeySize = 0;
            lastSectionIt = it;
        };

        while (it != std::end(traceInfo.second)) {
            const auto& row = *it;

            if (const auto sRow = dynamic_cast<const _SectionRow *>(row.get())) {
                setValueOffsets();
            } else if (const auto sRow = dynamic_cast<const _PropRow *>(row.get())) {
                longestKeySize = std::max(longestKeySize,
                                          static_cast<Index>(sRow->key.size()));
            }

            ++it;
        }

        setValueOffsets();
    }
}

void TraceInfoView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();
    assert(this->_index() < this->_rowCount());

    for (Index index = this->_index();
            index < this->_index() + this->contentRect().h; ++index) {
        if (index >= _rows->size()) {
            return;
        }

        const auto y = this->_contentRectYFromIndex(index);
        const auto& row = (*_rows)[index];

        if (const auto sRow = dynamic_cast<const _SectionRow *>(row.get())) {
            this->_moveCursor({0, y});
            this->_stylist().sectionTitle(*this);
            this->_safePrint("%s:", sRow->title.c_str());
        } else if (const auto sRow = dynamic_cast<const _PropRow *>(row.get())) {
            this->_moveCursor({2, y});
            this->_stylist().traceInfoViewPropKey(*this);
            this->_safePrint("%s:", sRow->key.c_str());
            this->_moveCursor({sRow->valueOffset + 2, y});
            this->_stylist().traceInfoViewPropValue(*this);

            if (const auto vRow = dynamic_cast<const _SignedIntPropRow *>(row.get())) {
                if (vRow->sepNumber) {
                    this->_safePrint("%s", utils::sepNumber(vRow->value, ',').c_str());
                } else {
                    this->_safePrint("%s", utils::sepNumber(vRow->value, ',').c_str());
                }
            } else if (const auto vRow = dynamic_cast<const _StringPropRow *>(row.get())) {
                this->_safePrint("%s", vRow->value.c_str());
            } else if (const auto vRow = dynamic_cast<const _DataSizePropRow *>(row.get())) {
                std::string size;
                std::string unit;

                std::tie(size, unit) = vRow->size.format();
                this->_safePrint("%s %s", size.c_str(), unit.c_str());
            } else if (const auto vRow = dynamic_cast<const _TimestampPropRow *>(row.get())) {
                this->_safePrint("%s", vRow->ts.format().c_str());
            } else if (const auto vRow = dynamic_cast<const _DurationPropRow *>(row.get())) {
                this->_safePrint("%s", vRow->duration.format().c_str());
            } else if (const auto vRow = dynamic_cast<const _NonePropRow *>(row.get())) {
                this->_stylist().tableViewNaCell(*this, false);
                this->_safePrint("N/A");
            }
        } else if (const auto sRow = dynamic_cast<const _SepRow *>(row.get())) {
            this->_moveCursor({2, y});
            this->_stylist().traceInfoViewPropKey(*this);
            this->_moveAndPrint({2, y}, "---");
        }
    }
}

void TraceInfoView::_stateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DATA_STREAM_FILE_CHANGED) {
        _rows = &_traceInfo[_state->metadata().traceType().get()];
        this->_index(0);
        this->_rowCount(_rows->size());
        this->_redrawContent();
    }
}

} // namespace jacques
