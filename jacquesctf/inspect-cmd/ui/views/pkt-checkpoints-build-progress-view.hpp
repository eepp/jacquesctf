/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP

#include "view.hpp"
#include "data/pkt-index-entry.hpp"
#include "data/er.hpp"

namespace jacques {

class PktCheckpointsBuildProgressView final :
    public View
{
public:
    explicit PktCheckpointsBuildProgressView(const Rect& rect, const Stylist& stylist);
    void pktIndexEntry(const PktIndexEntry& entry);
    void er(const Er& er);

protected:
    void _resized() override;
    void _redrawContent() override;

private:
    void _clearRow(Index y);
    void _drawProgress();
    void _drawPktIndexEntry();

private:
    const PktIndexEntry *_pktIndexEntry = nullptr;
    const Er *_er = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_CHECKPOINTS_BUILD_PROGRESS_VIEW_HPP
