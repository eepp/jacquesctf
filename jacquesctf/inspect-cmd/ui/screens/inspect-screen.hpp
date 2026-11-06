/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP
#define _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP

#include <list>

#include "aliases.hpp"
#include "../stylist.hpp"
#include "../../state/inspect-cmd-state.hpp"
#include "../../state/msg.hpp"
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
    using PktBookmarks = std::array<std::optional<Index>, 4>;
    using DsFileBookmarks = std::unordered_map<Index, PktBookmarks>;
    using Bookmarks = std::unordered_map<Index, DsFileBookmarks>;

public:
    explicit InspectScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                           ScreenHost& host);
    ~InspectScreen();

private:
    struct _StateSnapshot final
    {
        Index dsfStateIndex;
        std::optional<Index> pktIndexInDsFile;
        Index offsetInPktBits = 0;

        bool operator==(const _StateSnapshot& other)
        {
            return dsfStateIndex == other.dsfStateIndex &&
                   pktIndexInDsFile == other.pktIndexInDsFile &&
                   offsetInPktBits == other.offsetInPktBits;
        }
    };

private:
    struct _ViewRects final
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
    void _resized() override;
    void _handleKey(Key key) override;
    void _visibilityChanged() override;

    /*
     * Shows the decoding error box if the active packet has a decoding
     * error, or hides it otherwise.
     */
    void _tryShowDecodingError();

    /*
     * Called when the application state changes to show the decoding
     * error box if the new active packet has a decoding error.
     */
    void _appStateChanged(Message msg);

    Rect _decErrorViewRect() const noexcept;
    void _snapshotState();
    _StateSnapshot _takeStateSnapshot();
    void _goBack();
    void _goForward();
    void _restoreStateSnapshot(const _StateSnapshot& snapshot);
    void _updateViews();
    PktBookmarks& _curPktBookmarks() noexcept;
    void _toggleBookmark(unsigned int id);
    void _gotoBookmark(unsigned int id);
    void _setLastOffsetInRowBits();
    void _search(const SearchQuery& query, bool animate = true);

    /*
     * Opens the search box with the initial input text `init`; the
     * search happens when the user accepts the query (see
     * _searchAccepted()).
     *
     * If `live` is true, then the state follows the search input as
     * the user types it.
     */
    void _startSearch(const std::string& init, bool live);

    /*
     * Searches with the query `query` which the user accepted, if any.
     */
    void _searchAccepted(std::unique_ptr<const SearchQuery> query);

private:
    std::unique_ptr<ErTableView> _ertView;
    std::unique_ptr<PktDataView> _pdView;
    std::unique_ptr<PktRegionInfoView> _priView;
    std::unique_ptr<SubDtExplorerView> _sdteView;
    std::unique_ptr<PktDecodingErrorDetailsView> _decErrorView;
    SearchCtrl _searchCtrl;
    std::unique_ptr<const SearchQuery> _lastQuery;
    const Size _maxStateSnapshots = 500;
    std::list<_StateSnapshot> _stateSnapshots;
    decltype(_stateSnapshots)::iterator _curStateSnapshot;
    CycleWheel<_ErtViewDispMode> _ertViewDispModeWheel;
    bool _sdteViewIsVisible = true;
    Bookmarks _bookmarks;
    bool _goingToBookmark = false;
    std::optional<Index> _lastOffsetInRowBits;

    // last so that it's destroyed first
    InspectCmdStateObserverGuard _appStateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_SCREENS_INSPECT_SCREEN_HPP
