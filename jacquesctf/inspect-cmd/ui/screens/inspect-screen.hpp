/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP

#include <tuple>
#include <list>

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../../state/state.hpp"
#include "../views/pkt-region-info-view.hpp"
#include "../views/er-table-view.hpp"
#include "../views/sub-dt-explorer-view.hpp"
#include "../views/pkt-decoding-error-details-view.hpp"
#include "inspect-screen.hpp"
#include "screen.hpp"
#include "../cycle-wheel.hpp"
#include "../search-ctrl.hpp"

namespace jacques {

class PktDataView;

class InspectScreen final :
    public Screen
{
public:
    using PktBookmarks = std::array<boost::optional<Index>, 4>;
    using DsFileBookmarks = std::unordered_map<Index, PktBookmarks>;
    using Bookmarks = std::unordered_map<Index, DsFileBookmarks>;

public:
    explicit InspectScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                           State& state);
    ~InspectScreen();

private:
    struct _StateSnapshot
    {
        Index dsfStateIndex;
        boost::optional<Index> pktIndexInDsFile;
        Index offsetInPktBits;

        bool operator==(const _StateSnapshot& other)
        {
            return dsfStateIndex == other.dsfStateIndex &&
                   pktIndexInDsFile == other.pktIndexInDsFile &&
                   offsetInPktBits == other.offsetInPktBits;
        }
    };

private:
    struct _ViewRects
    {
        Rect ert;
        Rect pri;
        Rect pd;
    };

    enum class _ErtViewDispMode {
        HIDDEN,
        SHORT,
        LONG,
        FULL,
    };

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;
    void _tryShowDecodingError();
    void _snapshotState();
    _StateSnapshot _takeStateSnapshot();
    void _goBack();
    void _goForward();
    void _restoreStateSnapshot(const _StateSnapshot& snapshot);
    void _updateViews();
    PktBookmarks& _curPktBookmarks() noexcept;
    void _toggleBookmark(unsigned int id);
    void _gotoBookmark(unsigned int id);
    void _refreshViews();
    void _setLastOffsetInRowBits();
    void _search(const SearchQuery& query, bool animate = true);

private:
    std::unique_ptr<ErTableView> _ertView;
    std::unique_ptr<PktDataView> _pdView;
    std::unique_ptr<PktRegionInfoView> _priView;
    std::unique_ptr<SubDtExplorerView> _sdteView;
    std::unique_ptr<PktDecodingErrorDetailsView> _decErrorView;
    SearchCtrl _searchCtrl;
    std::unique_ptr<const SearchQuery> _lastQuery;
    CycleWheel<TsFmtMode> _tsFmtModeWheel;
    CycleWheel<utils::LenFmtMode> _dataLenFmtModeWheel;
    const Size _maxStateSnapshots = 500;
    std::list<_StateSnapshot> _stateSnapshots;
    decltype(_stateSnapshots)::iterator _curStateSnapshot;
    CycleWheel<_ErtViewDispMode> _ertViewDispModeWheel;
    bool _sdteViewIsVisible = true;
    Bookmarks _bookmarks;
    bool _goingToBookmark = false;
    boost::optional<Index> _lastOffsetInRowBits;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP
