/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>

#include "state.hpp"
#include "search-parser.hpp"
#include "message.hpp"
#include "metadata-error.hpp"
#include "active-data-stream-file-changed-message.hpp"

namespace jacques {

State::State(const std::list<boost::filesystem::path>& paths,
             std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener)
{
    assert(!paths.empty());

    MetadataStore metadataStore;

    for (const auto& path : paths) {
        const auto metadataPath = path.parent_path() / "metadata";
        auto metadataSp = metadataStore.metadata(metadataPath);

        if (metadataSp->invalidStreamError() ||
                metadataSp->invalidMetadataError() ||
                metadataSp->parseError()) {
            throw MetadataError {metadataSp};
        }

        auto dsfState = std::make_unique<DataStreamFileState>(*this, path,
                                                              metadataSp,
                                                              packetCheckpointsBuildListener);

        _dataStreamFileStates.push_back(std::move(dsfState));
    }

    _activeDataStreamFileStateIndex = 0;
    _activeDataStreamFileState = _dataStreamFileStates[0].get();
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
    this->_notify(ActiveDataStreamFileChangedMessage {});

    if (_activeDataStreamFileState->dataStreamFile().packetCount() > 0) {
        if (!_activeDataStreamFileState->hasActivePacket()) {
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

void State::_notify(const Message& msg)
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
