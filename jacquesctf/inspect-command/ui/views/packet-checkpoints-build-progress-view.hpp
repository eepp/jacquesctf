/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_COMMAND_UI_VIEWS_PACKET_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP
#define _JACQUES_INSPECT_COMMAND_UI_VIEWS_PACKET_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP

#include "view.hpp"
#include "packet-index-entry.hpp"
#include "event-record.hpp"

namespace jacques {

class PacketCheckpointsBuildProgressView :
    public View
{
public:
    explicit PacketCheckpointsBuildProgressView(const Rectangle& rect,
                                                const Stylist& stylist);
    void packetIndexEntry(const PacketIndexEntry& entry);
    void eventRecord(const EventRecord& eventRecord);

protected:
    void _resized() override;
    void _redrawContent() override;

private:
    void _clearRow(Index y);
    void _drawProgress();
    void _drawPacketIndexEntry();

private:
    const PacketIndexEntry *_packetIndexEntry = nullptr;
    const EventRecord *_eventRecord = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_COMMAND_UI_VIEWS_PACKET_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP
