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
#include "data/ts.hpp"
#include "data/time-ops.hpp"
#include "../stylist.hpp"
#include "../../state/msg.hpp"

namespace jacques {

TraceInfoView::TraceInfoView(const Rect& rect, const Stylist& stylist, State& state) :
    ScrollView {rect, "Trace info", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this}
{
    this->_buildRows();
    _rows = &_traceInfo[state.metadata().traceType().get()];
    this->_rowCount(_rows->size());
    this->_drawRows();
}

void TraceInfoView::_buildTraceInfoRows(const Metadata& metadata)
{
    auto& traceType = *metadata.traceType();

    if (_traceInfo.find(&traceType) != _traceInfo.end()) {
        return;
    }

    _Rows rows;

    rows.push_back(std::make_unique<_SectionRow>("Paths"));
    rows.push_back(std::make_unique<_StrPropRow>("Trace directory",
                                                 metadata.path().parent_path().string()));
    rows.push_back(std::make_unique<_StrPropRow>("Metadata stream", metadata.path().string()));
    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Data stream info"));

    Size dsfCount = 0;
    Size dsfWithoutDsIdCount = 0;
    DataLen totalEffectivePktsTotalLen = 0;
    DataLen totalEffectivePktsContentLen = 0;
    DataLen totalExpectedPktsTotalLen = 0;
    DataLen totalExpectedPktsContentLen = 0;
    boost::optional<Ts> firstTs;
    boost::optional<Ts> lastTs;
    boost::optional<Ts> intersectFirstTs;
    boost::optional<Ts> intersectLastTs;
    Size pktCount = 0;
    std::set<std::pair<Index, Index>> dsIds;

    for (const auto& dsfState : _state->dsFileStates()) {
        if (dsfState->metadata().traceType().get() != &traceType) {
            continue;
        }

        const auto& dsFile = dsfState->dsFile();

        ++dsfCount;
        pktCount += dsFile.pktCount();

        for (const auto& entry : dsFile.pktIndexEntries()) {
            if (entry.expectedContentLen()) {
                totalExpectedPktsContentLen += *entry.expectedContentLen();
            }

            if (entry.expectedTotalLen()) {
                totalExpectedPktsTotalLen += *entry.expectedTotalLen();
            }

            totalEffectivePktsContentLen += entry.effectiveContentLen();
            totalEffectivePktsTotalLen += entry.effectiveTotalLen();
        }

        if (dsFile.pktCount() > 0) {
            const auto& dsId = dsFile.pktIndexEntry(0).dsId();
            const auto dst = dsFile.pktIndexEntry(0).dst();

            if (dsId && dst) {
                dsIds.insert({dst->id(), *dsId});
            } else {
                ++dsfWithoutDsIdCount;
            }

            const auto& firstpktIndexEntry = dsFile.pktIndexEntries().front();
            const auto& lastpktIndexEntry = dsFile.pktIndexEntries().back();
            const auto& dsfFirstTs = firstpktIndexEntry.beginTs();
            const auto& dsfLastTs = lastpktIndexEntry.endTs();

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

    rows.push_back(std::make_unique<_SIntPropRow>("Data stream files",
                                                  static_cast<long long>(dsfCount)));
    rows.push_back(std::make_unique<_SIntPropRow>("Data streams",
                                                  static_cast<long long>(dsIds.size() + dsfWithoutDsIdCount)));
    rows.push_back(std::make_unique<_SIntPropRow>("Packets", static_cast<long long>(pktCount)));
    rows.push_back(std::make_unique<_SepRow>());
    rows.push_back(std::make_unique<_DataLenPropRow>("Effective total length of packets",
                                                     totalEffectivePktsTotalLen));
    rows.push_back(std::make_unique<_DataLenPropRow>("Effective total content length of packets",
                                                     totalEffectivePktsContentLen));
    rows.push_back(std::make_unique<_SIntPropRow>("Effective total content length (b) of packets",
                                                  static_cast<long long>(totalEffectivePktsContentLen.bits())));

    const auto totalEffectivePktsPaddingLen = totalEffectivePktsTotalLen -
                                              totalEffectivePktsContentLen;

    rows.push_back(std::make_unique<_DataLenPropRow>("Effective total padding length of packets",
                                                     totalEffectivePktsPaddingLen));
    rows.push_back(std::make_unique<_SIntPropRow>("Effective total padding length (b) of packets",
                                                  static_cast<long long>(totalEffectivePktsPaddingLen.bits())));
    rows.push_back(std::make_unique<_SepRow>());
    rows.push_back(std::make_unique<_DataLenPropRow>("Expected total length of packets",
                                                     totalExpectedPktsTotalLen));
    rows.push_back(std::make_unique<_DataLenPropRow>("Expected total content length of packets",
                                                     totalExpectedPktsContentLen));
    rows.push_back(std::make_unique<_SIntPropRow>("Expected total content length (b) of packets",
                                                  static_cast<long long>(totalExpectedPktsContentLen.bits())));

    const auto totalExpectedPktsPaddingLen = totalExpectedPktsTotalLen -
                                             totalExpectedPktsContentLen;

    rows.push_back(std::make_unique<_DataLenPropRow>("Expected total padding length packets",
                                                     totalExpectedPktsPaddingLen));
    rows.push_back(std::make_unique<_SIntPropRow>("Expected total padding length (b) of packets",
                                                  static_cast<long long>(totalExpectedPktsPaddingLen.bits())));
    rows.push_back(std::make_unique<_SepRow>());

    if (firstTs) {
        rows.push_back(std::make_unique<_TsPropRow>("Beginning", *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Beginning"));
    }

    if (lastTs) {
        rows.push_back(std::make_unique<_TsPropRow>("End", *lastTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("End"));
    }

    if (firstTs && lastTs && firstTs <= lastTs) {
        rows.push_back(std::make_unique<_DurationPropRow>("Duration", *lastTs - *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Duration"));
    }

    rows.push_back(std::make_unique<_SepRow>());

    if (intersectFirstTs && intersectLastTs &&
            *intersectFirstTs < *intersectLastTs) {
        rows.push_back(std::make_unique<_TsPropRow>("Intersection beginning", *intersectFirstTs));
        rows.push_back(std::make_unique<_TsPropRow>("Intersection end", *intersectLastTs));

        const auto intersectDuration = *intersectLastTs - *intersectFirstTs;

        rows.push_back(std::make_unique<_DurationPropRow>("Intersection duration",
                                                          intersectDuration));

        assert(firstTs);
        assert(lastTs);
        rows.push_back(std::make_unique<_SepRow>());

        const Duration disjointDuration {(*lastTs - *firstTs).ns() - intersectDuration.ns()};

        rows.push_back(std::make_unique<_DurationPropRow>("Beginning to intersection",
                                                          *intersectFirstTs - *firstTs));
        rows.push_back(std::make_unique<_DurationPropRow>("Intersection to end",
                                                          *lastTs - *intersectLastTs));
        rows.push_back(std::make_unique<_DurationPropRow>("Disjoint duration", disjointDuration));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("Intersection"));
    }

    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Metadata stream info"));

    rows.push_back(std::make_unique<_StrPropRow>("Packetized",
                                                 metadata.streamPktCount() ? "Yes" : "No"));
    rows.push_back(std::make_unique<_StrPropRow>("Path", metadata.path().string()));
    rows.push_back(std::make_unique<_DataLenPropRow>("Size", metadata.fileLen()));

    if (metadata.streamPktCount()) {
        const auto version = std::to_string(*metadata.streamMajorVersion()) + "." +
                             std::to_string(*metadata.streamMinorVersion());

        rows.push_back(std::make_unique<_StrPropRow>("Version", version));
        rows.push_back(std::make_unique<_SIntPropRow>("Packets",
                                                      static_cast<long long>(*metadata.streamPktCount())));

        const auto bo = (*metadata.streamBo() == yactfr::ByteOrder::BIG) ?
                        "Big endian" : "Little endian";

        rows.push_back(std::make_unique<_StrPropRow>("Byte order", bo));
        rows.push_back(std::make_unique<_StrPropRow>("UUID",
                                                     boost::uuids::to_string(*metadata.streamUuid())));
    }

    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Trace type"));

    const auto version = std::to_string(traceType.majorVersion()) + "." +
                         std::to_string(traceType.minorVersion());

    rows.push_back(std::make_unique<_StrPropRow>("Version", version));

    if (traceType.uuid()) {
        rows.push_back(std::make_unique<_StrPropRow>("UUID",
                                                     boost::uuids::to_string(*traceType.uuid())));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>("UUID"));
    }

    rows.push_back(std::make_unique<_StrPropRow>("Is correlatable",
                                                 metadata.isCorrelatable() ? "Yes" : "No"));

    rows.push_back(std::make_unique<_SIntPropRow>("Data stream types",
                                                  static_cast<long long>(traceType.dataStreamTypes().size())));

    const auto ertCount = std::accumulate(traceType.dataStreamTypes().begin(),
                                          traceType.dataStreamTypes().end(), 0ULL,
                                          [](const auto curCount, const auto& dst) {
        return curCount + dst->eventRecordTypes().size();
    });

    rows.push_back(std::make_unique<_SIntPropRow>("Event record types",
                                                  static_cast<long long>(ertCount)));
    rows.push_back(std::make_unique<_SIntPropRow>("Clock types",
                                                  static_cast<long long>(traceType.clockTypes().size())));

    if (!traceType.env().entries().empty()) {
        rows.push_back(std::make_unique<_EmptyRow>());
        rows.push_back(std::make_unique<_SectionRow>("Trace type environment"));

        std::vector<const yactfr::TraceTypeEnv::Entries::value_type *> entries;

        for (auto& entryPair : traceType.env().entries()) {
            entries.push_back(&entryPair);
        }

        std::sort(entries.begin(), entries.end(),
                  [](const auto entryLeft, const auto entryRight) {
            return entryLeft->first < entryRight->first;
        });

        for (const auto& entryPair : entries) {
            if (const auto intEntry = boost::get<long long>(&entryPair->second)) {
                rows.push_back(std::make_unique<_SIntPropRow>(entryPair->first, *intEntry));
            } else if (const auto strEntry = boost::get<std::string>(&entryPair->second)) {
                rows.push_back(std::make_unique<_StrPropRow>(entryPair->first, *strEntry));
            }
        }
    }

    for (auto& dst : traceType.dataStreamTypes()) {
        rows.push_back(std::make_unique<_EmptyRow>());

        const auto title = std::string {"Data stream type "} + std::to_string(dst->id());

        rows.push_back(std::make_unique<_SectionRow>(title));
        rows.push_back(std::make_unique<_SIntPropRow>("Event record types",
                                                      static_cast<long long>(dst->eventRecordTypes().size())));
    }

    for (auto& clkType : traceType.clockTypes()) {
        rows.push_back(std::make_unique<_EmptyRow>());

        const auto title = std::string {"Clock type `"} + clkType->name() + "`";

        rows.push_back(std::make_unique<_SectionRow>(title));

        if (clkType->description()) {
            rows.push_back(std::make_unique<_StrPropRow>("Description", *clkType->description()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>("Description"));
        }

        if (clkType->uuid()) {
            rows.push_back(std::make_unique<_StrPropRow>("UUID",
                                                         boost::uuids::to_string(*clkType->uuid())));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>("UUID"));
        }

        rows.push_back(std::make_unique<_SIntPropRow>("Frequency (Hz)",
                                                      static_cast<long long>(clkType->freq())));
        rows.push_back(std::make_unique<_SIntPropRow>("Offset (s)",
                                                      static_cast<long long>(clkType->offset().seconds())));
        rows.push_back(std::make_unique<_SIntPropRow>("Offset (cycles)",
                                                      static_cast<long long>(clkType->offset().cycles())));
        rows.push_back(std::make_unique<_SIntPropRow>("Error (cycles)",
                                                      static_cast<long long>(clkType->error())));
        rows.push_back(std::make_unique<_StrPropRow>("Is absolute",
                                                     clkType->isAbsolute() ? "Yes" : "No"));
    }

    _traceInfo[&traceType] = std::move(rows);
}

void TraceInfoView::_buildRows()
{
    for (const auto& dsfState : _state->dsFileStates()) {
        this->_buildTraceInfoRows(dsfState->metadata());
    }

    for (const auto& traceInfo : _traceInfo) {
        Size longestKeySize = 0;
        auto it = traceInfo.second.begin();
        auto lastSectionIt = it;

        const auto setValOffsets = [&longestKeySize, &lastSectionIt, &it]() {
            while (lastSectionIt != it) {
                if (const auto sRow = dynamic_cast<_PropRow *>(lastSectionIt->get())) {
                    sRow->valOffset = longestKeySize + 2;
                }

                ++lastSectionIt;
            }

            longestKeySize = 0;
            lastSectionIt = it;
        };

        while (it != traceInfo.second.end()) {
            const auto& row = *it;

            if (const auto sRow = dynamic_cast<const _SectionRow *>(row.get())) {
                setValOffsets();
            } else if (const auto sRow = dynamic_cast<const _PropRow *>(row.get())) {
                longestKeySize = std::max(longestKeySize, static_cast<Index>(sRow->key.size()));
            }

            ++it;
        }

        setValOffsets();
    }
}

void TraceInfoView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();
    assert(this->_index() < this->_rowCount());

    for (Index index = this->_index(); index < this->_index() + this->contentRect().h; ++index) {
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
            this->_moveCursor({sRow->valOffset + 2, y});
            this->_stylist().traceInfoViewPropVal(*this);

            if (const auto vRow = dynamic_cast<const _SIntPropRow *>(row.get())) {
                if (vRow->sepNumber) {
                    this->_safePrint("%s", utils::sepNumber(vRow->val, ',').c_str());
                } else {
                    this->_safePrint("%s", utils::sepNumber(vRow->val, ',').c_str());
                }
            } else if (const auto vRow = dynamic_cast<const _StrPropRow *>(row.get())) {
                this->_safePrint("%s", vRow->val.c_str());
            } else if (const auto vRow = dynamic_cast<const _DataLenPropRow *>(row.get())) {
                std::string len;
                std::string unit;

                std::tie(len, unit) = vRow->len.format();
                this->_safePrint("%s %s", len.c_str(), unit.c_str());
            } else if (const auto vRow = dynamic_cast<const _TsPropRow *>(row.get())) {
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
    if (msg == Message::ACTIVE_DS_FILE_CHANGED) {
        _rows = &_traceInfo[_state->metadata().traceType().get()];
        this->_index(0);
        this->_rowCount(_rows->size());
        this->_redrawContent();
    }
}

} // namespace jacques
