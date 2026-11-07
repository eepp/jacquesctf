/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <numeric>
#include <set>
#include <string>
#include <cassert>
#include <variant>
#include <functional>
#include <fmt/format.h>

#include "../stylist.hpp"
#include "../term.hpp"
#include "pkt-region-info-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"
#include "data/content-pkt-region.hpp"
#include "data/padding-pkt-region.hpp"
#include "data/error-pkt-region.hpp"

namespace jacques {

PktRegionInfoView::PktRegionInfoView(const Rect& rect, const Stylist& stylist,
                                     InspectCmdState& appState) :
    View {rect, "Packet region info", DecorationStyle::BORDERLESS, stylist},
    ViewInspectCmdStateObserver {appState, *this}
{
    this->_setMaxDtPathSizes();
}

void PktRegionInfoView::_appStateChanged(Message)
{
    this->_invalidate();
}

namespace {

const char *scopeStr(const yactfr::Scope scope) noexcept
{
    switch (scope) {
    case yactfr::Scope::PacketHeader:
        return "PH";

    case yactfr::Scope::PacketContext:
        return "PC";

    case yactfr::Scope::EventRecordHeader:
        return "ERH";

    case yactfr::Scope::EventRecordCommonContext:
        return "ERCC";

    case yactfr::Scope::EventRecordSpecificContext:
        return "ERSC";

    case yactfr::Scope::EventRecordPayload:
        return "ERP";

    default:
        std::abort();
    }
}

class DtPathItemStrVisitor final
{
public:
    using Result = std::pair<std::string, std::string>;

public:
    explicit DtPathItemStrVisitor(ContentPktRegion::ArrayIndexes::const_iterator * const arrayIndexesIt) :
        _arrayIndexesIt {arrayIndexesIt}
    {
    }

    template <typename ItemT>
    Result operator()(const ItemT& item) const
    {
        return this->_itemStr(item);
    }

private:
    Result _itemStr(const DtPath::StructMemberItem& item) const
    {
        return std::pair {std::string {"/"}, utils::escapeStr(item.name)};
    }

    Result _itemStr(const DtPath::VarOptItem& item) const
    {
        return std::pair {std::string {}, std::invoke([&item] {
            if (item.name) {
                return std::string {'<'} + utils::escapeStr(*item.name) + '>';
            } else {
                return fmt::format("<{}>", item.index);
            }
        })};
    }

    Result _itemStr(const DtPath::CurArrayElemItem&) const
    {
        /*
         * A bit of a hack here: we also use this visitor to compute the
         * maximum possible length of a data path string, but at this
         * point we don't know the maximum array indexes yet.
         *
         * Reserve five digits each as a trade-off.
         */
        const auto index = _arrayIndexesIt ? **_arrayIndexesIt : 99999;
        auto str = fmt::format("[{}]", index);

        if (_arrayIndexesIt) {
            ++*_arrayIndexesIt;
        }

        return std::pair {std::string {}, str};
    }

    Result _itemStr(const DtPath::CurOptDataItem&) const
    {
        return std::pair {std::string {}, std::string {"<%>"}};
    }

private:
    ContentPktRegion::ArrayIndexes::const_iterator *_arrayIndexesIt;
};

DtPathItemStrVisitor::Result dtPathItemStr(const DtPath::Item& item,
                                           ContentPktRegion::ArrayIndexes::const_iterator * const arrayIndexesIt)
{
    return std::visit(DtPathItemStrVisitor {arrayIndexesIt}, item);
}

} // namespace

void PktRegionInfoView::_redrawContent()
{
    this->_clearContent(Stylist::Role::INFO);

    const auto pktRegion = this->_appState().curPktRegion();

    if (!pktRegion) {
        return;
    }

    const ContentPktRegion *cPktRegion = nullptr;
    bool isError = false;
    Index x = 0;

    if ((cPktRegion = dynamic_cast<const ContentPktRegion *>(pktRegion))) {
        // path
        if (cPktRegion->dtPath().items().empty()) {
            this->_style(Stylist::Role::INFO_STRONG);
        }

        x = this->_write({x, 0}, scopeStr(cPktRegion->dtPath().scope()));

        if (cPktRegion->dtPath().items().empty()) {
            this->_style(Stylist::Role::INFO);
        }

        auto arrayIndexesIt = cPktRegion->arrayIndexes().begin();

        for (auto it = cPktRegion->dtPath().items().begin(); it != cPktRegion->dtPath().items().end(); ++it) {
            const auto [prefix, str] = dtPathItemStr(*it, &arrayIndexesIt);

            x = this->_write({x, 0}, prefix);

            if (it == cPktRegion->dtPath().items().end() - 1) {
                this->_style(Stylist::Role::INFO_STRONG);
            }

            x = this->_write({x, 0}, str);
        }
    } else if (dynamic_cast<const PaddingPktRegion *>(pktRegion)) {
        if (pktRegion->scope()) {
            this->_style(Stylist::Role::INFO);
            x = this->_write({x, 0}, scopeStr(pktRegion->scope()->scope()));
            x = this->_write({x, 0}, " ");
        }

        this->_style(Stylist::Role::INFO_STRONG);
        this->_write({x, 0}, "PADDING");
    } else if (dynamic_cast<const ErrorPktRegion *>(pktRegion)) {
        this->_style(Stylist::Role::INFO_STRONG);
        this->_write({x, 0}, "ERROR");
        isError = true;
    }

    // size
    this->_style(Stylist::Role::INFO);

    const auto pathWidth = _maxDtPathSizes[&this->_appState().trace()];
    const auto str = utils::sepNumber(pktRegion->segment().len()->bits(), ',');

    x = this->_write({pathWidth + 4 + this->_curMaxOffsetSize() - 2 - str.size(), 0}, "{} b", str);

    // byte order
    if (pktRegion->segment().bo()) {
        x = this->_write({x, 0}, "    ");

        if (*pktRegion->segment().bo() == yactfr::ByteOrder::Big) {
            x = this->_write({x, 0}, "BE");
        } else {
            x = this->_write({x, 0}, "LE");
        }
    } else {
        x = this->_write({x, 0}, "      ");
    }

    // value
    if (cPktRegion && cPktRegion->val()) {
        const auto& varVal = *cPktRegion->val();

        x = this->_write({x, 0}, "    ");
        this->_style(Stylist::Role::INFO_VAL);

        if (const auto val = std::get_if<bool>(&varVal)) {
            x = this->_write({x, 0}, *val ? "true" : "false");
        } else if (const auto val = std::get_if<long long>(&varVal)) {
            x = this->_write({x, 0}, utils::sepNumber(*val, ','));
        } else if (const auto val = std::get_if<unsigned long long>(&varVal)) {
            const auto prefDispBase = std::invoke([cPktRegion] {
                if (cPktRegion->dt().isIntegerType()) {
                    return utils::intTypePrefDispBase(cPktRegion->dt());
                } else if (cPktRegion->dt().isFixedLengthBitArrayType()) {
                    return yactfr::DisplayBase::Hexadecimal;
                } else {
                    return yactfr::DisplayBase::Decimal;
                }
            });

            switch (prefDispBase) {
            case yactfr::DisplayBase::Octal:
                x = this->_write({x, 0}, "0{:o}", *val);
                break;

            case yactfr::DisplayBase::Hexadecimal:
                x = this->_write({x, 0}, "0x{:x}", *val);
                break;

            default:
                x = this->_write({x, 0}, utils::sepNumber(*val, ','));
                break;
            }
        } else if (const auto val = std::get_if<double>(&varVal)) {
            x = this->_write({x, 0}, "{:f}", *val);
        } else if (const auto val = std::get_if<std::string>(&varVal)) {
            x = this->_write({x, 0}, utils::escapeStr(*val));
        }

        if (cPktRegion->dt().isFixedLengthBitMapType() || cPktRegion->dt().isIntegerType()) {
            const auto names = cPktRegion->flagOrMappingNames();

            if (!names.empty()) {
                this->_style(Stylist::Role::INFO);
                x = this->_write({x, 0}, "   ");

                for (auto& name : names) {
                    this->_style(Stylist::Role::INFO);
                    x = this->_write({x, 0}, " [");
                    this->_style(Stylist::Role::INFO_STRONG);
                    x = this->_write({x, 0}, name);
                    this->_style(Stylist::Role::INFO);
                    x = this->_write({x, 0}, "]");
                }
            }
        }
    } else if (isError) {
        const auto& error = this->_appState().activePktState().pkt().error();

        assert(error);
        x = this->_write({x, 0}, "    ");
        this->_style(Stylist::Role::INFO_ERROR);
        this->_write({x, 0}, utils::escapeStr(error->decodingError().what()));
    }
}

void PktRegionInfoView::_setMaxDtPathSize(const Trace& trace)
{
    const auto accFunc = [](const auto total, auto& item) {
        const auto [prefix, str] = dtPathItemStr(item, nullptr);

        return total + prefix.size() + term::strWidth(str);
    };

    const auto totalDtPathItSizeFunc = [&accFunc](auto& dtDtPathPair) {
        /*
         * Adding `dtDtPathPair.second.items().size()` for the
         * separators and 4 for the scope name.
         */
        return std::accumulate(dtDtPathPair.second.items().begin(),
                               dtDtPathPair.second.items().end(), 0ULL, accFunc) +
               dtDtPathPair.second.items().size() + 4;
    };

    const auto maxDtPathIt = std::max_element(trace.metadata().dtPaths().begin(),
                                              trace.metadata().dtPaths().end(),
                                              [&totalDtPathItSizeFunc](const auto& pairA,
                                                                       const auto& pairB) {
        return totalDtPathItSizeFunc(pairA) < totalDtPathItSizeFunc(pairB);
    });

    _maxDtPathSizes.emplace(&trace,
                            maxDtPathIt == trace.metadata().dtPaths().end() ? 0ULL :
                            totalDtPathItSizeFunc(*maxDtPathIt));
}

void PktRegionInfoView::_setMaxDtPathSizes()
{
    for (const auto& dsfState : this->_appState().dsFileStates()) {
        auto& trace = dsfState->trace();

        if (_maxDtPathSizes.find(&trace) == _maxDtPathSizes.end()) {
            this->_setMaxDtPathSize(trace);
        }
    }
}

Size PktRegionInfoView::_curMaxOffsetSize()
{
    assert(this->_appState().hasActivePktState());

    const auto& pkt = this->_appState().activePktState().pkt();
    const auto it = _maxOffsetSizes.find(&pkt);

    if (it == _maxOffsetSizes.end()) {
        const auto str = utils::sepNumber(pkt.indexEntry().effectiveTotalLen().bits());
        const auto size = str.size();

        _maxOffsetSizes[&pkt] = size;
        return size;
    } else {
        return it->second;
    }
}

} // namespace jacques
