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
#include "data/trace.hpp"
#include "search-query.hpp"
#include "msg.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

State::State(const std::vector<bfs::path>& paths,
             std::shared_ptr<PktCheckpointsBuildListener> pktCheckpointsBuildListener)
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

        for (auto& dsFile : trace->dsFiles()) {
            _dsFileStates.push_back(std::make_unique<DsFileState>(*this, *dsFile,
                                                                  pktCheckpointsBuildListener));
        }

        _traces.push_back(std::move(trace));
    }

    _activeDsFileStateIndex = 0;
    _activeDsFileState = _dsFileStates.front().get();
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

void State::gotoDsFile(const Index index)
{
    assert(index < _dsFileStates.size());

    if (index == _activeDsFileStateIndex) {
        return;
    }

    _activeDsFileStateIndex = index;
    _activeDsFileState = _dsFileStates[index].get();
    this->_notify(Message::ACTIVE_DS_FILE_CHANGED);

    if (_activeDsFileState->dsFile().pktCount() > 0) {
        if (!_activeDsFileState->hasActivePktState()) {
            _activeDsFileState->gotoPkt(0);
        }
    }
}

void State::gotoPrevDsFile()
{
    if (_activeDsFileStateIndex == 0) {
        return;
    }

    this->gotoDsFile(_activeDsFileStateIndex - 1);
}

void State::gotoNextDsFile()
{
    if (_activeDsFileStateIndex == _dsFileStates.size() - 1) {
        return;
    }

    this->gotoDsFile(_activeDsFileStateIndex + 1);
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
    return this->activeDsFileState().search(query);
}

StateObserverGuard::StateObserverGuard(State& state, const State::Observer& observer) :
    _state {&state},
    _observerId {_state->addObserver(observer)}
{
}

StateObserverGuard::~StateObserverGuard()
{
    _state->removeObserver(_observerId);
}

} // namespace jacques
