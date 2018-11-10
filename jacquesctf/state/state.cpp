/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <map>

#include "state.hpp"
#include "trace.hpp"
#include "search-parser.hpp"
#include "message.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

State::State(const std::list<bfs::path>& paths,
             std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener)
{
    assert(!paths.empty());

    std::map<bfs::path, std::vector<bfs::path>> tracePaths;

    // group by trace
    for (const auto& path : paths) {
        tracePaths[path.parent_path()].push_back(path);
    }

    // create traces
    for (const auto& tracePathPathsPair : tracePaths) {
        auto trace = std::make_unique<Trace>(tracePathPathsPair.second);

        for (auto& dataStreamFile : trace->dataStreamFiles()) {
            auto dsfState = std::make_unique<DataStreamFileState>(*this,
                                                                  *dataStreamFile,
                                                                  packetCheckpointsBuildListener);
            _dataStreamFileStates.push_back(std::move(dsfState));
        }

        _traces.push_back(std::move(trace));
    }

    _activeDataStreamFileStateIndex = 0;
    _activeDataStreamFileState = _dataStreamFileStates.front().get();
}

Index State::addObserver(const Observer& observer)
{
    const Index id = _observers.size();

    _observers.push_back(observer);
    return id;
}

void State::removeObserver(const Index id)
{
    assert(id < _observers.size());
    _observers[id] = nullptr;
}

void State::gotoDataStreamFile(const Index index)
{
    assert(index < _dataStreamFileStates.size());

    if (index == _activeDataStreamFileStateIndex) {
        return;
    }

    _activeDataStreamFileStateIndex = index;
    _activeDataStreamFileState = _dataStreamFileStates[index].get();
    this->_notify(Message::ACTIVE_DATA_STREAM_FILE_CHANGED);

    if (_activeDataStreamFileState->dataStreamFile().packetCount() > 0) {
        if (!_activeDataStreamFileState->hasActivePacketState()) {
            _activeDataStreamFileState->gotoPacket(0);
        }
    }
}

void State::gotoPreviousDataStreamFile()
{
    if (_activeDataStreamFileStateIndex == 0) {
        return;
    }

    this->gotoDataStreamFile(_activeDataStreamFileStateIndex - 1);
}

void State::gotoNextDataStreamFile()
{
    if (_activeDataStreamFileStateIndex == _dataStreamFileStates.size() - 1) {
        return;
    }

    this->gotoDataStreamFile(_activeDataStreamFileStateIndex + 1);
}

void State::_notify(const Message msg)
{
    for (const auto& observer : _observers) {
        if (observer) {
            observer(msg);
        }
    }
}

bool State::search(const SearchQuery& query)
{
    return this->activeDataStreamFileState().search(query);
}

StateObserverGuard::StateObserverGuard(State& state,
                                       const State::Observer& observer) :
    _state {&state}
{
    _observerId = _state->addObserver(observer);
}

StateObserverGuard::~StateObserverGuard()
{
    _state->removeObserver(_observerId);
}

} // namespace jacques
