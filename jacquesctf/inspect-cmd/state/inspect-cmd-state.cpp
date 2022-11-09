/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <map>

#include "inspect-cmd-state.hpp"
#include "msg.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

InspectCmdState::InspectCmdState(const std::vector<bfs::path>& paths,
                                 PktCheckpointsBuildListener& pktCheckpointsBuildListener) :
    AppState {paths, pktCheckpointsBuildListener}
{
}

Index InspectCmdState::addObserver(const Observer& observer)
{
    const Index id = _observers.size();

    _observers.push_back(observer);
    return id;
}

void InspectCmdState::removeObserver(const Index id)
{
    assert(id < _observers.size());
    _observers[id] = nullptr;
}

void InspectCmdState::_notify(const Message msg)
{
    for (const auto& observer : _observers) {
        if (observer) {
            observer(msg);
        }
    }
}

void InspectCmdState::_activeDsFileAndPktChanged()
{
    this->_notify(Message::ACTIVE_DS_FILE_AND_PKT_CHANGED);
}

void InspectCmdState::_activePktChanged()
{
    this->_notify(Message::ACTIVE_PKT_CHANGED);
}

void InspectCmdState::_curOffsetInPktChanged()
{
    this->_notify(Message::CUR_OFFSET_IN_PKT_CHANGED);
}

InspectCmdStateObserverGuard::InspectCmdStateObserverGuard(InspectCmdState& appState,
                                                           const InspectCmdState::Observer& observer) :
    _appState {&appState},
    _observerId {_appState->addObserver(observer)}
{
}

InspectCmdStateObserverGuard::~InspectCmdStateObserverGuard()
{
    _appState->removeObserver(_observerId);
}

} // namespace jacques
