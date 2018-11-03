/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_SCREEN_HPP
#define _JACQUES_INSPECT_SCREEN_HPP

#include <tuple>
#include <list>

#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "packet-region-info-view.hpp"
#include "event-record-table-view.hpp"
#include "packet-data-view.hpp"
#include "packet-decoding-error-details-view.hpp"
#include "inspect-screen.hpp"
#include "screen.hpp"
#include "cycle-wheel.hpp"
#include "search-controller.hpp"

namespace jacques {

class InspectScreen :
    public Screen
{
public:
    explicit InspectScreen(const Rectangle& rect, const Config& cfg,
                           const Stylist& stylist, State& state);

private:
    struct _StateSnapshot
    {
        Index dsfStateIndex;
        boost::optional<Index> packetIndexInDataStreamFile;
        Index offsetInPacketBits;

        bool operator==(const _StateSnapshot& other)
        {
            return dsfStateIndex == other.dsfStateIndex &&
                   packetIndexInDataStreamFile == other.packetIndexInDataStreamFile &&
                   offsetInPacketBits == other.offsetInPacketBits;
        }
    };

private:
    struct _ViewRects
    {
        Rectangle ert;
        Rectangle prInfo;
        Rectangle pd;
    };

    enum class _ErtViewDisplayMode {
        HIDDEN,
        SHORT,
        LONG,
    };

    enum class _ {
        HIDDEN,
        SHORT,
        LONG,
    };

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;
    void _tryShowDecodingError();
    void _snapshotState();
    void _goBack();
    void _goForward();
    void _restoreStateSnapshot(const _StateSnapshot& snapshot);
    void _updateViews();

private:
    std::unique_ptr<EventRecordTableView> _ertView;
    std::unique_ptr<PacketDataView> _pdView;
    std::unique_ptr<PacketRegionInfoView> _prInfoView;
    std::unique_ptr<PacketDecodingErrorDetailsView> _decErrorView;
    SearchController _searchController;
    std::unique_ptr<const SearchQuery> _lastQuery;
    CycleWheel<TimestampFormatMode> _tsFormatModeWheel;
    CycleWheel<utils::SizeFormatMode> _dsFormatModeWheel;
    const Size _maxStateSnapshots = 500;
    std::list<_StateSnapshot> _stateSnapshots;
    decltype(_stateSnapshots)::iterator _currentStateSnapshot;
    CycleWheel<_ErtViewDisplayMode> _ertViewDisplayModeWheel;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_SCREEN_HPP
