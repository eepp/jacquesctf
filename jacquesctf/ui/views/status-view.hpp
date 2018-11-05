/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_STATUS_VIEW_HPP
#define _JACQUES_STATUS_VIEW_HPP

#include <unordered_map>

#include "view.hpp"

namespace jacques {

class DataStreamFileState;

class StatusView :
    public View
{
public:
    explicit StatusView(const Rectangle& rect,
                        const Stylist& stylist, State& state);

private:
    struct _EndPositions
    {
        Index packetCount;
        Index packetIndex;
        Index seqNum;
        Index curOffsetInPacketBits;
        Index dsfPath;
    };

private:
    void _createEndPositions();
    void _drawOffset();
    void _stateChanged(const Message& msg) override;
    void _redrawContent() override;

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
    std::unordered_map<const DataStreamFileState *, _EndPositions> _endPositions;
    const _EndPositions *_curEndPositions = nullptr;
};

} // namespace jacques

#endif // _JACQUES_STATUS_VIEW_HPP
