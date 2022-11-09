/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <map>

#include "data/trace.hpp"
#include "app-state.hpp"
#include "search-query.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

AppState::AppState(const std::vector<bfs::path>& paths,
                   PktCheckpointsBuildListener& pktCheckpointsBuildListener)
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

void AppState::gotoDsFile(const Index index)
{
    assert(index < _dsFileStates.size());

    if (index == _activeDsFileStateIndex) {
        return;
    }

    _activeDsFileStateIndex = index;
    _activeDsFileState = _dsFileStates[index].get();
    this->_activeDsFileChanged();

    if (_activeDsFileState->dsFile().pktCount() > 0) {
        if (!_activeDsFileState->hasActivePktState()) {
            _activeDsFileState->gotoPkt(0);
        }
    }
}

void AppState::gotoPrevDsFile()
{
    if (_activeDsFileStateIndex == 0) {
        return;
    }

    this->gotoDsFile(_activeDsFileStateIndex - 1);
}

void AppState::gotoNextDsFile()
{
    if (_activeDsFileStateIndex == _dsFileStates.size() - 1) {
        return;
    }

    this->gotoDsFile(_activeDsFileStateIndex + 1);
}

bool AppState::search(const SearchQuery& query)
{
    return this->activeDsFileState().search(query);
}

void AppState::_activeDsFileChanged()
{
}

void AppState::_activePktChanged()
{
}

void AppState::_curOffsetInPktChanged()
{
}

} // namespace jacques
