/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_SCREEN_HPP
#define _JACQUES_INSPECT_SCREEN_HPP

#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "event-record-table-view.hpp"
#include "packet-decoding-error-details-view.hpp"
#include "inspect-screen.hpp"
#include "screen.hpp"
#include "cycle-wheel.hpp"

namespace jacques {

class InspectScreen :
    public Screen
{
public:
    explicit InspectScreen(const Rectangle& rect, const Config& cfg,
                           std::shared_ptr<const Stylist> stylist,
                           std::shared_ptr<State> state);

protected:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    void _tryShowDecodingError();

private:
    std::unique_ptr<EventRecordTableView> _ertView;
    std::unique_ptr<PacketDecodingErrorDetailsView> _decErrorView;
    CycleWheel<TimestampFormatMode> _tsFormatModeWheel;
    CycleWheel<utils::SizeFormatMode> _dsFormatModeWheel;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_SCREEN_HPP
