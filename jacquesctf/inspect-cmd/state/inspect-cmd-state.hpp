/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_INSPECT_CMD_STATE_HPP
#define _JACQUES_INSPECT_CMD_STATE_INSPECT_CMD_STATE_HPP

#include <vector>
#include <functional>
#include <boost/filesystem.hpp>

#include "msg.hpp"
#include "inspect-common/app-state.hpp"
#include "data/ts.hpp"
#include "utils.hpp"
#include "../ui/cycle-wheel.hpp"

namespace jacques {

class InspectCmdState final :
    public AppState
{
public:
    using Observer = std::function<void (Message)>;

public:
    explicit InspectCmdState(const std::vector<boost::filesystem::path>& paths,
                             PktCheckpointsBuildListener& pktCheckpointsBuildListener);
    Index addObserver(const Observer& observer);
    void removeObserver(Index id);

    /*
     * Current timestamp format mode.
     */
    TsFmtMode tsFmtMode() const noexcept
    {
        return _tsFmtModeWheel.curVal();
    }

    /*
     * Current data length format mode.
     */
    utils::LenFmtMode dataLenFmtMode() const noexcept
    {
        return _dataLenFmtModeWheel.curVal();
    }

    /*
     * Selects the next timestamp format mode (possibly wrapping), and
     * notifies `Message::FMT_MODES_CHANGED`.
     */
    void nextTsFmtMode();

    /*
     * Selects the next data length format mode (possibly wrapping), and
     * notifies `Message::FMT_MODES_CHANGED`.
     */
    void nextDataLenFmtMode();

private:
    void _notify(Message msg);
    void _activeDsFileAndPktChanged() override;
    void _activePktChanged() override;
    void _curOffsetInPktChanged() override;

private:
    std::vector<Observer> _observers;

    CycleWheel<TsFmtMode> _tsFmtModeWheel {
        TsFmtMode::LONG,
        TsFmtMode::NS_FROM_ORIGIN,
        TsFmtMode::CYCLES,
    };

    CycleWheel<utils::LenFmtMode> _dataLenFmtModeWheel {
        utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BITS,
    };
};

class InspectCmdStateObserverGuard final
{
public:
    explicit InspectCmdStateObserverGuard(InspectCmdState& appState,
                                          const InspectCmdState::Observer& observer);
    ~InspectCmdStateObserverGuard();

private:
    InspectCmdState *_appState;
    Index _observerId;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_STATE_INSPECT_CMD_STATE_HPP
