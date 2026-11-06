/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_STATUS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_STATUS_VIEW_HPP

#include <unordered_map>

#include "view.hpp"

namespace jacques {

class DsFileState;

class StatusView final :
    public View,
    private ViewInspectCmdStateObserver
{
public:
    explicit StatusView(const Rect& rect, const Stylist& stylist, InspectCmdState& appState);

private:
    struct _EndPositions final
    {
        Index pktCount;
        Index pktIndex;
        Index seqNum;
        Index pktPercent;
        Index curOffsetInDsFileBits;
        Index curOffsetInPktBits;
        Index dsfPath;
    };

private:
    void _createEndPositions();
    void _drawOffset();
    void _appStateChanged(Message msg) override;
    void _redrawContent() override;

private:
    std::unordered_map<const DsFileState *, _EndPositions> _endPositions;
    const _EndPositions *_curEndPositions = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_STATUS_VIEW_HPP
