/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILES_SCREEN_HPP
#define _JACQUES_DATA_STREAM_FILES_SCREEN_HPP

#include "aliases.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "data-stream-file-table-view.hpp"
#include "screen.hpp"
#include "cycle-wheel.hpp"
#include "data-size.hpp"

namespace jacques {

class DataStreamFilesScreen :
    public Screen
{
public:
    explicit DataStreamFilesScreen(const Rectangle& rect,
                                   const InspectConfig& cfg,
                     		       const Stylist& stylist, State& state);

private:
    void _redraw() override;
    void _resized() override;
    KeyHandlingReaction _handleKey(int key) override;
    void _visibilityChanged() override;

private:
    std::unique_ptr<DataStreamFileTableView> _view;
    CycleWheel<TimestampFormatMode> _tsFormatModeWheel;
    CycleWheel<utils::SizeFormatMode> _dsFormatModeWheel;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILES_SCREEN_HPP
