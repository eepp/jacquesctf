/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_DECODING_ERROR_DETAILS_VIEW_HPP
#define _JACQUES_PACKET_DECODING_ERROR_DETAILS_VIEW_HPP

#include <string>

#include "view.hpp"
#include "packet-checkpoints.hpp"

namespace jacques {

class PacketDecodingErrorDetailsView :
    public View
{
public:
    explicit PacketDecodingErrorDetailsView(const Rectangle& rect,
                                            std::shared_ptr<const Stylist> stylist,
                                            std::shared_ptr<State> state);

private:
    void _redrawContent() override;
    void _stateChanged(const Message& msg) override;

private:
    std::shared_ptr<State> _state;
    const ViewStateObserverGuard _stateObserverGuard;

};

} // namespace jacques

#endif // _JACQUES_PACKET_DECODING_ERROR_DETAILS_VIEW_HPP
