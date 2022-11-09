/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_STATE_INSPECT_CMD_STATE_HPP
#define _JACQUES_INSPECT_CMD_STATE_INSPECT_CMD_STATE_HPP

#include <vector>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/core/noncopyable.hpp>

#include "msg.hpp"
#include "inspect-common/app-state.hpp"

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

private:
    void _notify(Message msg);
    void _activeDsFileChanged() override;
    void _activePktChanged() override;
    void _curOffsetInPktChanged() override;

private:
    std::vector<Observer> _observers;
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
