/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_REGION_INFO_VIEW_HPP
#define _JACQUES_PACKET_REGION_INFO_VIEW_HPP

#include "view.hpp"

namespace jacques {

class PacketRegionInfoView :
    public View
{
public:
    explicit PacketRegionInfoView(const Rectangle& rect,
                                const Stylist& stylist, State& state);

private:
    void _stateChanged(const Message& msg) override;
    void _redrawContent() override;
    void _safePrintScope(yactfr::Scope scope);

private:
    State * const _state;
    const ViewStateObserverGuard _stateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_PACKET_REGION_INFO_VIEW_HPP
