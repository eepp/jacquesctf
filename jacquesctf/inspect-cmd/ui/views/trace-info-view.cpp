/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <variant>
#include <boost/uuid/uuid_io.hpp>
#include <yactfr/yactfr.hpp>

#include "trace-info-view.hpp"
#include "data/ts.hpp"
#include "data/time-ops.hpp"
#include "../stylist.hpp"
#include "../term.hpp"
#include "../../state/msg.hpp"

namespace jacques {

TraceInfoView::TraceInfoView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState) :
    ScrollView {rect, "Trace info", DecorationStyle::BORDERS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_buildRows();
    _rows = &_traceInfo[&appState.trace()];
    this->_rowCount(_rows->size());
}

void TraceInfoView::_buildTraceInfoRows(const Trace& trace)
{
    if (_traceInfo.find(&trace) != _traceInfo.end()) {
        return;
    }

    auto& metadata = trace.metadata();
    auto& traceType = metadata.traceType();

    _Rows rows;

    rows.push_back(std::make_unique<_SectionRow>("Paths"));
    rows.push_back(std::make_unique<_StrPropRow>("Trace directory",
                                                 metadata.path().parent_path().string()));
    rows.push_back(std::make_unique<_StrPropRow>("Metadata stream", metadata.path().string()));
    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Data streams"));

    Size dsfCount = 0;
    Size dsfWithoutDsIdCount = 0;
    DataLen totalEffectivePktsTotalLen = 0;
    DataLen totalEffectivePktsContentLen = 0;
    DataLen totalExpectedPktsTotalLen = 0;
    DataLen totalExpectedPktsContentLen = 0;
    std::optional<Ts> firstTs;
    std::optional<Ts> lastTs;
    std::optional<Ts> intersectFirstTs;
    std::optional<Ts> intersectLastTs;
    Size pktCount = 0;
    std::set<std::pair<Index, Index>> dsIds;

    for (auto& dsFile : trace.dsFiles()) {
        ++dsfCount;
        pktCount += dsFile->pktCount();

        for (const auto& entry : dsFile->pktIndexEntries()) {
            if (entry.expectedContentLen()) {
                totalExpectedPktsContentLen += *entry.expectedContentLen();
            }

            if (entry.expectedTotalLen()) {
                totalExpectedPktsTotalLen += *entry.expectedTotalLen();
            }

            totalEffectivePktsContentLen += entry.effectiveContentLen();
            totalEffectivePktsTotalLen += entry.effectiveTotalLen();
        }

        if (dsFile->pktCount() > 0) {
            const auto& dsId = dsFile->pktIndexEntry(0).dsId();
            const auto dst = dsFile->pktIndexEntry(0).dst();

            if (dsId && dst) {
                dsIds.emplace(dst->id(), *dsId);
            } else {
                ++dsfWithoutDsIdCount;
            }

            const auto& firstpktIndexEntry = dsFile->pktIndexEntries().front();
            const auto& lastpktIndexEntry = dsFile->pktIndexEntries().back();
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

    static constexpr const char *beginStr = "Beginning";

    if (firstTs) {
        rows.push_back(std::make_unique<_TsPropRow>(beginStr, *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>(beginStr));
    }

    static constexpr const char *endStr = "End";

    if (lastTs) {
        rows.push_back(std::make_unique<_TsPropRow>(endStr, *lastTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>(endStr));
    }

    static constexpr const char *durationStr = "Duration";

    if (firstTs && lastTs && firstTs <= lastTs) {
        rows.push_back(std::make_unique<_DurationPropRow>(durationStr, *lastTs - *firstTs));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>(durationStr));
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
    rows.push_back(std::make_unique<_SectionRow>("Metadata stream"));

    const auto pMetadataStream = dynamic_cast<const yactfr::PacketizedMetadataStream *>(&metadata.stream());

    rows.push_back(std::make_unique<_StrPropRow>("Packetized", pMetadataStream ? "Yes" : "No"));
    rows.push_back(std::make_unique<_StrPropRow>("Path", metadata.path().string()));
    rows.push_back(std::make_unique<_DataLenPropRow>("Size", metadata.fileLen()));

    if (pMetadataStream) {
        rows.push_back(std::make_unique<_SIntPropRow>("Packets",
                                                      static_cast<long long>(pMetadataStream->packetCount())));

        const auto bo = (pMetadataStream->byteOrder() == yactfr::ByteOrder::Big) ?
                        "Big-endian" : "Little-endian";

        rows.push_back(std::make_unique<_StrPropRow>("Byte order", bo));
        rows.push_back(std::make_unique<_StrPropRow>("UUID (stream packets)",
                                                     boost::uuids::to_string(pMetadataStream->uuid())));
    } else if (metadata.streamUuid()) {
        rows.push_back(std::make_unique<_StrPropRow>("Stream UUID",
                                                     boost::uuids::to_string(*metadata.streamUuid())));
    }

    rows.push_back(std::make_unique<_EmptyRow>());
    rows.push_back(std::make_unique<_SectionRow>("Trace type"));

    const auto version = std::to_string(traceType.majorVersion()) + "." +
                         std::to_string(traceType.minorVersion());

    rows.push_back(std::make_unique<_StrPropRow>("Version", version));

    static constexpr const char *traceUuidStr = "Trace UID";

    if (traceType.uid()) {
        rows.push_back(std::make_unique<_StrPropRow>(traceUuidStr, *traceType.uid()));
    } else {
        rows.push_back(std::make_unique<_NonePropRow>(traceUuidStr));
    }

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

    if (!traceType.environment().entries().empty()) {
        rows.push_back(std::make_unique<_EmptyRow>());
        rows.push_back(std::make_unique<_SectionRow>("Trace environment"));

        for (auto& [key, val] : traceType.environment().entries()) {
            if (const auto intEntry = std::get_if<long long>(&val)) {
                rows.push_back(std::make_unique<_SIntPropRow>(key, *intEntry));
            } else if (const auto strEntry = std::get_if<std::string>(&val)) {
                rows.push_back(std::make_unique<_StrPropRow>(key, *strEntry));
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
        assert(clkType->internalId());

        const auto title = std::string {"Clock type `"} + *clkType->internalId() + "`";

        rows.push_back(std::make_unique<_SectionRow>(title));

        static constexpr const char *descrStr = "Description";

        if (clkType->description()) {
            rows.push_back(std::make_unique<_StrPropRow>(descrStr, *clkType->description()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(descrStr));
        }

        static constexpr const char *nsStr = "Namespace";

        if (clkType->nameSpace()) {
            rows.push_back(std::make_unique<_StrPropRow>(nsStr, *clkType->nameSpace()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(nsStr));
        }

        static constexpr const char *nameStr = "Name";

        if (clkType->name()) {
            rows.push_back(std::make_unique<_StrPropRow>(nameStr, *clkType->name()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(nameStr));
        }

        static constexpr const char *uidStr = "UID";

        if (clkType->uid()) {
            rows.push_back(std::make_unique<_StrPropRow>(uidStr, *clkType->uid()));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(uidStr));
        }

        static constexpr const char *origUuidStr = "Original UUID";

        if (clkType->originalUuid()) {
            rows.push_back(std::make_unique<_StrPropRow>(origUuidStr,
                                                         boost::uuids::to_string(*clkType->originalUuid())));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(origUuidStr));
        }

        static constexpr const char *origStr = "Origin";

        if (clkType->origin()) {
            if (clkType->origin()->isUnixEpoch()) {
                rows.push_back(std::make_unique<_StrPropRow>(origStr, "Unix epoch"));
            } else {
                static constexpr const char *originNsStr = "Origin namespace";

                if (clkType->origin()->nameSpace()) {
                    rows.push_back(std::make_unique<_StrPropRow>(originNsStr, *clkType->origin()->nameSpace()));
                } else {
                    rows.push_back(std::make_unique<_NonePropRow>(originNsStr));
                }

                rows.push_back(std::make_unique<_StrPropRow>("Origin name", clkType->origin()->name()));
                rows.push_back(std::make_unique<_StrPropRow>("Origin UID", clkType->origin()->uid()));
            }
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(origStr));
        }

        rows.push_back(std::make_unique<_SIntPropRow>("Frequency (Hz)",
                                                      static_cast<long long>(clkType->frequency())));
        rows.push_back(std::make_unique<_SIntPropRow>("Offset from origin (s)",
                                                      static_cast<long long>(clkType->offsetFromOrigin().seconds())));
        rows.push_back(std::make_unique<_SIntPropRow>("Offset from origin (cycles)",
                                                      static_cast<long long>(clkType->offsetFromOrigin().cycles())));

        static constexpr const char *precStr = "Precision (cycles)";

        if (clkType->precision()) {
            rows.push_back(std::make_unique<_SIntPropRow>(precStr,
                                                          static_cast<long long>(*clkType->precision())));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(precStr));
        }

        static constexpr const char *accuracyStr = "Accuracy (cycles)";

        if (clkType->accuracy()) {
            rows.push_back(std::make_unique<_SIntPropRow>(accuracyStr,
                                                          static_cast<long long>(*clkType->accuracy())));
        } else {
            rows.push_back(std::make_unique<_NonePropRow>(accuracyStr));
        }
    }

    _traceInfo[&trace] = std::move(rows);
}

void TraceInfoView::_buildRows()
{
    for (const auto& dsfState : _appState->dsFileStates()) {
        this->_buildTraceInfoRows(dsfState->dsFile().trace());
    }

    for (const auto& [trace, rows] : _traceInfo) {
        Size longestKeyWidth = 0;
        auto it = rows.begin();
        auto lastSectionIt = it;

        const auto setValOffsets = [&longestKeyWidth, &lastSectionIt, &it]() {
            while (lastSectionIt != it) {
                if (const auto sRow = dynamic_cast<_PropRow *>(lastSectionIt->get())) {
                    sRow->valOffset = longestKeyWidth + 2;
                }

                ++lastSectionIt;
            }

            longestKeyWidth = 0;
            lastSectionIt = it;
        };

        while (it != rows.end()) {
            const auto& row = *it;

            if (const auto sRow = dynamic_cast<const _SectionRow *>(row.get())) {
                setValOffsets();
            } else if (const auto sRow = dynamic_cast<const _PropRow *>(row.get())) {
                longestKeyWidth = std::max(longestKeyWidth, term::strWidth(sRow->key));
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
            } else if (const auto vRow = dynamic_cast<const _ErrorPropRow *>(row.get())) {
                this->_stylist().error(*this);
                this->_safePrint("%s", vRow->val.c_str());
            } else if (const auto vRow = dynamic_cast<const _StrPropRow *>(row.get())) {
                this->_safePrint("%s", vRow->val.c_str());
            } else if (const auto vRow = dynamic_cast<const _DataLenPropRow *>(row.get())) {
                const auto [len, unit] = vRow->len.format();

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

void TraceInfoView::_appStateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_AND_PKT_CHANGED) {
        _rows = &_traceInfo[&_appState->trace()];
        this->_index(0);
        this->_rowCount(_rows->size());
    }
}

} // namespace jacques
